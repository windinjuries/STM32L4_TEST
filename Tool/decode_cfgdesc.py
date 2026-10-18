#!/usr/bin/env python3
"""Decode and validate the USB configuration descriptor actually served to the host.

Usage:
    python Tool/decode_cfgdesc.py <file>                # txt / csv / hex dump
    python Tool/decode_cfgdesc.py --selftest            # check the decoder itself

Input formats accepted (auto-detected, first match wins):
    * GDB `dump binary memory` output  -> pass the file directly
    * GDB `x/166xb` output             -> one line per byte, or several per line
    * one long hex string, or space separated hex bytes

The decoder reports the things that make a host refuse a configuration:
    * wTotalLength disagreement
    * bNumInterfaces vs interfaces actually present
    * bNumEndpoints per interface vs interface/endpoint descriptor counts
    * duplicate endpoint addresses
    * endpoint address <-> transfer type mismatches
    * audio terminal topology (playback vs capture) and channel agreement
"""
import re
import sys

KIND_NAMES = {1: "DEVICE", 2: "CONFIG", 3: "STRING", 4: "INTERFACE", 5: "ENDPOINT",
              0x21: "AUDIO_IF", 0x24: "AUDIO_CLASS_IF", 0x25: "AUDIO_CLASS_EP"}

SUBTYPE_AC = {1: "HEADER", 2: "INPUT_TERMINAL", 3: "OUTPUT_TERMINAL",
              6: "FEATURE_UNIT"}
SUBTYPE_AS = {1: "AS_GENERAL", 2: "FORMAT_TYPE"}

TERMINAL_TYPES = {
    0x0101: "USB Streaming", 0x0102: "USB Vendor Specific",
    0x0201: "Microphone", 0x0202: "Desktop Microphone",
    0x0301: "Speaker", 0x0302: "Headphones",
}


def parse_hex_dump(text):
    """Extract bytes from GDB x/Nxb output or a flat hex string."""
    data = bytearray()
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        # GDB memory dump: "0x20008a88 <sym>:  0x12  0x01 ..."
        body = line
        if ":" in line:
            body = line.split(":", 1)[1]
        body = body.replace("<", " ").replace(">", " ")
        toks = re.findall(r"0x([0-9a-fA-F]{1,2})\b", body)
        if toks and ":" in line:
            for t in toks:
                data.append(int(t, 16))
            continue
        # flat hex string like "1201000200..."
        flat = re.sub(r"[^0-9a-fA-F]", "", body)
        if len(flat) >= 2 and (":" not in line):
            if len(flat) % 2 == 0 and len(flat) >= 4:
                for i in range(0, len(flat), 2):
                    data.append(int(flat[i:i + 2], 16))
    return bytes(data)


def load(path):
    raw = open(path, "rb").read()
    # A GDB binary dump is raw bytes already; a text dump has printable chars.
    printable = sum(1 for b in raw[:512] if 32 <= b < 127)
    if len(raw) > 0 and printable * 100 // max(1, len(raw[:512])) > 90:
        return parse_hex_dump(raw.decode("latin-1"))
    return raw


def _synthetic_cfg(capture):
    """Build the audio part of the configuration descriptor the firmware emits.

    capture=True  -> the UAC1 microphone topology this firmware builds
    capture=False -> the previous, broken one: Output Terminal typed 0x0302
                     (Headphones) and bTerminalLink pointing at the Microphone
                     *input* terminal, with a 1-channel Feature Unit for a
                     2-channel function. Windows reported code 10 on it.
    """
    if capture:
        ac = bytes([
            0x09, 0x24, 0x01, 0x00, 0x01, 0x28, 0x00, 0x01, 0x01,
            0x0C, 0x24, 0x02, 0x01, 0x01, 0x02, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
            0x0A, 0x24, 0x06, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
            0x09, 0x24, 0x03, 0x03, 0x01, 0x01, 0x00, 0x02, 0x00,
        ])
        link = 0x03
    else:
        ac = bytes([
            0x09, 0x24, 0x01, 0x00, 0x01, 0x27, 0x00, 0x01, 0x01,
            0x0C, 0x24, 0x02, 0x01, 0x01, 0x02, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
            0x09, 0x24, 0x06, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00,
            0x09, 0x24, 0x03, 0x03, 0x02, 0x03, 0x00, 0x02, 0x00,
        ])
        link = 0x01

    audio = bytearray([0x08, 0x0B, 0x00, 0x02, 0x01, 0x01, 0x00, 0x00,       # IAD
                       0x09, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00])  # AC IF
    audio += ac
    audio += bytes([0x09, 0x04, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,  # AS alt 0
                    0x09, 0x04, 0x01, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00,  # AS alt 1
                    0x07, 0x24, 0x01, link, 0x01, 0x01, 0x00,              # AS general
                    0x0B, 0x24, 0x02, 0x01, 0x02, 0x02, 0x10, 0x01,       # format
                    0x80, 0xBB, 0x00,                                      # 48000 Hz
                    0x09, 0x05, 0x81, 0x01, 0xC1, 0x00, 0x01, 0x00, 0x00,  # iso IN 193 B
                    0x07, 0x25, 0x01, 0x00, 0x00, 0x00, 0x00])            # CS endpoint

    total = 9 + len(audio)
    header = bytes([0x09, 0x02, total & 0xFF, (total >> 8) & 0xFF,
                    0x02, 0x01, 0x00, 0x80, 0x32])   # 2 interfaces, bus powered
    return header + bytes(audio)


def selftest():
    """The decoder must accept the new topology and reject the broken one."""
    import contextlib
    import io

    ok = True
    with contextlib.redirect_stdout(io.StringIO()):
        rc = parse(_synthetic_cfg(capture=True))
    if rc != 0:
        ok = False
        print("selftest: the microphone topology the firmware emits was rejected")

    with contextlib.redirect_stdout(io.StringIO()):
        rc = parse(_synthetic_cfg(capture=False))
    if rc == 0:
        ok = False
        print("selftest: the broken topology (0x0302 / bTerminalLink=1 / "
              "1-channel Feature Unit) was accepted")

    print("selftest %s" % ("OK" if ok else "FAILED"))
    return 0 if ok else 1


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    if sys.argv[1] == "--selftest":
        try:
            return selftest()
        except Exception as exc:  # noqa: BLE001
            print("selftest FAILED: %s" % exc)
            return 1

    data = load(sys.argv[1])
    print("loaded %d bytes" % len(data))
    if len(data) < 9 or data[1] != 0x02:
        print("!! first descriptor is not a CONFIGURATION descriptor "
              "(bDescriptorType=%s)" % (data[1] if len(data) > 1 else "?"))
        return 1
    return parse(data)


def parse(data):
    total = data[2] | (data[3] << 8)
    num_if = data[4]
    print("=" * 62)
    print("CONFIGURATION  wTotalLength=%d (0x%04x)  bNumInterfaces=%d  "
          "bConfigurationValue=%d  bmAttributes=0x%02x  bMaxPower=%d"
          % (total, total, num_if, data[5], data[7], data[8]))
    print("=" * 62)

    off = 9
    iface = None
    problems = []
    seen_eps = []
    ifaces = []
    audio_term = {}      # ("in"|"out", terminal id) -> wTerminalType
    audio_source = {}    # terminal/unit id -> bSourceID (for topology walk)
    format_ch = None
    it_channels = None
    as_terminal_link = None

    while off + 2 <= len(data):
        blen = data[off]
        if blen < 2:
            problems.append("offset %d: bLength=%d is invalid, stopping" % (off, blen))
            break
        if off + blen > len(data):
            problems.append("offset %d: descriptor (len %d) runs past end of buffer"
                            % (off, blen))
            break
        btype = data[off + 1]
        body = data[off:off + blen]
        kind = KIND_NAMES.get(btype, "type0x%02x" % btype)

        if btype == 0x04:  # INTERFACE
            if iface is not None:
                ifaces.append(iface)
            iface = {"num": body[2], "alt": body[3], "neps": body[4],
                     "cls": body[5], "sub": body[6], "proto": body[7], "off": off}
            print("IF %d alt %d  nEndpoints=%d  class=0x%02x sub=0x%02x proto=0x%02x"
                  % (body[2], body[3], body[4], body[5], body[6], body[7]))
        elif btype == 0x05:  # ENDPOINT
            addr = body[2]
            attr = body[3] & 0x03
            mps = body[4] | (body[5] << 8)
            ivl = body[6]
            xfer = {0: "Control", 1: "Isochronous", 2: "Bulk", 3: "Interrupt"}[attr]
            print("   EP 0x%02x %-3s %-12s wMaxPacketSize=%d bInterval=%d"
                  % (addr, "IN" if addr & 0x80 else "OUT", xfer, mps, ivl))
            # Alternate settings share the interface number but own their
            # endpoints, so endpoints are tracked per (interface, alt).
            seen_eps.append((iface["num"] if iface else -1,
                             iface["alt"] if iface else -1, addr, attr, mps))
        elif btype == 0x24:  # AUDIO class-specific interface descriptor
            # Subtype numbers mean different things in the AudioControl
            # (bInterfaceSubClass 0x01) and AudioStreaming (0x02) interfaces:
            # subtype 1 is the AC header there and AS_GENERAL here, subtype 2 is
            # the Input Terminal there and the Format Type here.
            ac_if = iface is not None and iface["sub"] == 0x01
            as_if = iface is not None and iface["sub"] == 0x02
            sub = body[2]
            name = SUBTYPE_AC.get(sub) or SUBTYPE_AS.get(sub) or "sub%d" % sub
            if ac_if and sub == 2 and blen >= 12:      # INPUT_TERMINAL
                tid = body[3]
                ttype = body[4] | (body[5] << 8)
                ch = body[7]
                it_channels = ch
                audio_term[("in", tid)] = ttype
                audio_source[tid] = 0
                print("   AC INPUT_TERMINAL id=%d type=0x%04x(%s) bNrChannels=%d"
                      % (tid, ttype, TERMINAL_TYPES.get(ttype, "?"), ch))
            elif ac_if and sub == 3 and blen >= 9:    # OUTPUT_TERMINAL
                tid = body[3]
                ttype = body[4] | (body[5] << 8)
                src = body[7]
                audio_term[("out", tid)] = ttype
                audio_source[tid] = src
                print("   AC OUTPUT_TERMINAL id=%d type=0x%04x(%s) bSourceID=%d"
                      % (tid, ttype, TERMINAL_TYPES.get(ttype, "?"), src))
            elif ac_if and sub == 6 and blen >= 7:    # FEATURE_UNIT
                uid = body[3]
                src = body[4]
                csize = body[5]
                nctl = blen - 7
                audio_source[uid] = src
                print("   AC FEATURE_UNIT id=%d bSourceID=%d bControlSize=%d "
                      "bmaControls=%d byte(s)" % (uid, src, csize, nctl))
                if it_channels is not None and csize:
                    need = csize * (it_channels + 1)
                    if need != nctl:
                        problems.append(
                            "FEATURE_UNIT id=%d has %d control byte(s) but "
                            "bControlSize=%d x (bNrChannels=%d + 1) needs %d; the "
                            "host reads the missing bytes out of the next descriptor"
                            % (uid, nctl, csize, it_channels, need))
            elif as_if and sub == 1 and blen >= 7:    # AS_GENERAL
                as_terminal_link = body[3]
                print("   AS GENERAL bTerminalLink=%d bDelay=%d wFormatTag=0x%04x"
                      % (body[3], body[4], body[5] | (body[6] << 8)))
            elif as_if and sub == 2 and blen >= 5:    # FORMAT_TYPE
                format_ch = body[4]
                print("   AS FORMAT_TYPE bFormatType=%d bNrChannels=%d "
                      "bSubFrameSize=%d bBitResolution=%d"
                      % (body[3], body[4], body[5], body[6]))
            else:
                print("   AC/AS %s (len %d)" % (name, blen))
        elif btype == 0x25:  # AUDIO class-specific endpoint
            print("   AS ENDPOINT bmAttributes=0x%02x" % body[3])
        else:
            print("   %s (len %d) %s" % (kind, blen, body[2:].hex()))

        off += blen

    if iface is not None:
        ifaces.append(iface)

    print("-" * 62)
    for i in ifaces:
        got = len([e for e in seen_eps if e[0] == i["num"] and e[1] == i["alt"]])
        if i["neps"] != got:
            problems.append("IF %d alt %d: bNumEndpoints=%d but %d endpoint "
                            "descriptor(s) follow" % (i["num"], i["alt"], i["neps"], got))

    if total != len(data):
        problems.append("wTotalLength=%d but %d bytes present" % (total, len(data)))
    if num_if != len(set(i["num"] for i in ifaces)):
        problems.append("bNumInterfaces=%d but %d distinct interface number(s)"
                        % (num_if, len(set(i["num"] for i in ifaces))))

    for idx, (ifn, alt, addr, attr, mps) in enumerate(seen_eps):
        for ifn2, alt2, addr2, _, _ in seen_eps[idx + 1:]:
            if addr == addr2:
                problems.append("endpoint 0x%02x declared more than once "
                                "(IF %d alt %d and IF %d alt %d)"
                                % (addr, ifn, alt, ifn2, alt2))

    print("interfaces: %s" % sorted(set(i["num"] for i in ifaces)))
    print("endpoints : %s" % ["0x%02x" % e[2] for e in seen_eps])

    # --- audio topology ----------------------------------------------------
    # The host resolves the stream behind an AudioStreaming interface through
    # bTerminalLink and then follows bSourceID back to the source. A capture
    # (microphone) function must link to a USB Streaming OUTPUT terminal
    # (wTerminalType 0x0101); a playback function links to the USB Streaming
    # INPUT terminal. When the link or a bSourceID cannot be resolved the class
    # driver fails with:
    #     "This device cannot start. (Code 10) - The object was not found."
    iso_in = [e for e in seen_eps if (e[2] & 0x80) and e[3] == 1]
    if as_terminal_link is not None:
        if ("out", as_terminal_link) in audio_term:
            direction = "capture"
            link_type = audio_term[("out", as_terminal_link)]
        elif ("in", as_terminal_link) in audio_term:
            direction = "playback"
            link_type = audio_term[("in", as_terminal_link)]
        else:
            direction = "dangling"
            link_type = None
            problems.append(
                "bTerminalLink=%d does not name any terminal of the audio function; "
                "the host cannot resolve the stream (code 10, object not found)"
                % as_terminal_link)
        if link_type is not None:
            print("audio topology: AS links to %s terminal %d, type 0x%04x (%s) -> %s"
                  % ("OUTPUT" if direction == "capture" else "INPUT",
                     as_terminal_link, link_type,
                     TERMINAL_TYPES.get(link_type, "?"), direction.upper()))
            if link_type != 0x0101:
                problems.append(
                    "AS bTerminalLink=%d names terminal type 0x%04x (%s), but an "
                    "audio streaming interface must link to the USB Streaming "
                    "terminal (0x0101)"
                    % (as_terminal_link, link_type, TERMINAL_TYPES.get(link_type, "?")))
            elif direction == "playback" and iso_in:
                problems.append(
                    "audio topology is PLAYBACK (USB Streaming input terminal) but "
                    "the streaming endpoint is an IN endpoint")
            elif direction == "capture" and not iso_in:
                problems.append(
                    "audio topology is CAPTURE but there is no isochronous IN endpoint")

    for uid in sorted(audio_source):
        src = audio_source[uid]
        if src and src not in audio_source:
            problems.append("unit/terminal id=%d references bSourceID=%d which does "
                            "not exist (code 10, object not found)" % (uid, src))

    if format_ch is not None:
        print("format channels: %d" % format_ch)
        if it_channels is not None and format_ch != it_channels:
            problems.append("Format Type I declares %d channel(s) while the Input "
                            "Terminal declares %d" % (format_ch, it_channels))

    print("=" * 62)
    if problems:
        print("PROBLEMS (%d):" % len(problems))
        for p in problems:
            print("  - %s" % p)
        return 1
    print("no structural problems found")
    return 0


if __name__ == "__main__":
    sys.exit(main())
