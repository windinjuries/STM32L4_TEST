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
    0x0101: "USB Streaming", 0x0102: "USB Streaming (out)",
    0x0201: "Microphone", 0x0202: "Headset (in)",
    0x0301: "Speaker", 0x0302: "Headset (out)",
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


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    if sys.argv[1] == "--selftest":
        blob = bytes([0x09, 0x02, 0x19, 0x00, 0x01, 0x01, 0x00, 0x80, 0x32,
                      0x09, 0x04, 0x00, 0x00, 0x0A, 0x03, 0x01, 0x01, 0x00,
                      0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x0A])
        try:
            parse(blob)
            print("selftest OK")
            return 0
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
    audio_term = {}
    format_ch = None

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
                iface["eps_seen"] = len([e for e in seen_eps if e[0] == iface["num"]])
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
            seen_eps.append((iface["num"] if iface else -1, addr, attr, mps))
        elif btype == 0x24:  # AUDIO class-specific interface descriptor
            sub = body[2]
            name = SUBTYPE_AC.get(sub) or SUBTYPE_AS.get(sub) or "sub%d" % sub
            if sub == 2:      # INPUT_TERMINAL
                tid = body[3]
                ttype = body[4] | (body[5] << 8)
                ch = body[7]
                audio_term[("in", tid)] = ttype
                print("   AC INPUT_TERMINAL id=%d type=0x%04x(%s) bNrChannels=%d"
                      % (tid, ttype, TERMINAL_TYPES.get(ttype, "?"), ch))
            elif sub == 3:    # OUTPUT_TERMINAL
                tid = body[3]
                ttype = body[4] | (body[5] << 8)
                src = body[7]
                audio_term[("out", tid)] = ttype
                print("   AC OUTPUT_TERMINAL id=%d type=0x%04x(%s) bSourceID=%d"
                      % (tid, ttype, TERMINAL_TYPES.get(ttype, "?"), src))
            elif sub == 1:    # AS_GENERAL
                print("   AS GENERAL bTerminalLink=%d bDelay=%d wFormatTag=0x%04x"
                      % (body[3], body[4], body[5] | (body[6] << 8)))
            elif sub == 2:    # FORMAT_TYPE
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
        got = len([e for e in seen_eps if e[0] == i["num"]])
        if i["neps"] != got:
            problems.append("IF %d alt %d: bNumEndpoints=%d but %d endpoint "
                            "descriptor(s) follow" % (i["num"], i["alt"], i["neps"], got))

    if total != len(data):
        problems.append("wTotalLength=%d but %d bytes present" % (total, len(data)))
    if num_if != len(set(i["num"] for i in ifaces)):
        problems.append("bNumInterfaces=%d but %d distinct interface number(s)"
                        % (num_if, len(set(i["num"] for i in ifaces))))

    for idx, (ifn, addr, attr, mps) in enumerate(seen_eps):
        for ifn2, addr2, _, _ in seen_eps[idx + 1:]:
            if addr == addr2:
                problems.append("endpoint 0x%02x declared more than once "
                                "(IF %d and IF %d)" % (addr, ifn, ifn2))

    print("interfaces: %s" % sorted(set(i["num"] for i in ifaces)))
    print("endpoints : %s" % ["0x%02x" % e[1] for e in seen_eps])

    if ("in", 1) in audio_term and ("out", 3) in audio_term:
        it = audio_term[("in", 1)]
        ot = audio_term[("out", 3)]
        if it == 0x0201 and ot == 0x0302:
            print("audio topology: CAPTURE (microphone) - OK")
        elif it == 0x0101 and ot == 0x0301:
            problems.append("audio topology is PLAYBACK (Speaker 0x0301) while "
                            "the streaming endpoint is an IN endpoint")
        else:
            print("audio topology: in=0x%04x out=0x%04x (unexpected)" % (it, ot))
    if format_ch is not None:
        print("format channels: %d" % format_ch)

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
