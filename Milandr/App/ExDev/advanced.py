from tabnanny import check


def read_packets(file_path):
    packets = list()
    with open(file_path, 'r') as f:
        for line in f:
            packet = [int(ch, 16) for ch in line.split(' ')]
            packets.append(packet)
    return packets

def to_converter(sub_list):

    out = list()

    if  len(sub_list) == 4:

        out0 = (((sub_list[0] & 0x80) >> 4) +
                ((sub_list[1] & 0x80) >> 5) +
                ((sub_list[2] & 0x80) >> 6) +
                ((sub_list[3] & 0x80) >> 7))

        out.append(out0)

        for j in range(0, 4):
            out.append(sub_list[j] & 0x7F)

        for j in range(0, 5):
            if out[j] < 48:
                out[j] ^= 0xCA


    return out

def from_host(sub_list):

    out = list([0,0,0,0])

    if len(sub_list) == 5:

        for j in range(0, 5):
            if sub_list[j] ^ 0xCA < 48:
                sub_list[j] ^= 0xCA

        out[0] = (sub_list[0] << 4) & 0x80
        out[1] = (sub_list[0] << 5) & 0x80
        out[2] = (sub_list[0] << 6) & 0x80
        out[3] = (sub_list[0] << 7) & 0x80

        for j in range(0, 4):
            out[j] |= (sub_list[j + 1] & 0x7F)

    return out

def to_host(sub_list):

    out = list([0,0,0,0,0])

    for j in range(0, 4):
        out[j] = sub_list[j] & 0x7F
        out[4] |= ( ( ( sub_list[j] >> 7 ) & 0x01 ) << j )

        if ( sub_list[j] ^ 0xCA ) & 0x80:
            out[j] ^= 0xCA

    return out

def from_converter(sub_list):

    out = list()

    if len(sub_list) == 5:

        for j in range(0, 5):
            if (sub_list[j] & 0x80) != 0:
                sub_list[j] = (sub_list[j] ^ 0xCA)

        for j in range(0, 4):
            out.append(sub_list[j] | (((sub_list[4] >> j) & 0x01) << 7))

    return out


def md_calc(__command__):
    summ = 0
    for j in range(2, len(__command__) - 1):
        summ += __command__[j]

    return 0x100 - summ & 0xFF

def md_check(__command__):
    summ = 0
    for j in range(1, len(__command__) - 1):
        summ += __command__[j]

    return summ & 0xFF == 0

commands = read_packets('packets.txt')

for command in commands:
    command4 = list([command[0]])
    command5 = list([command[0]])
    command4_from_conv = list([command[0]])

    for i in range(1, len(command) - 1 , 5):
        out4 = from_host(command[i:i+5])
        command4 += out4

        out5 = to_host(out4)
        command5 += out5

        in4 = from_converter(out5)

        command4_from_conv += in4

    command4.append(command[len(command) - 1])
    command5.append(command[len(command) - 1])
    command4_from_conv.append(command[len(command) - 1])

    md = md_calc(command4)

    original = ' '.join(hex(item).removeprefix('0x').rjust(2, '0') for item in command)
    original_to_converter = ' '.join(hex(item).removeprefix('0x').rjust(2,'0') for item in command4)
    converter_to_host = ' '.join(hex(item).removeprefix('0x').rjust(2, '0') for item in command5)
    from_converter_str = ' '.join(hex(item).removeprefix('0x').rjust(2, '0') for item in command4_from_conv)

    print( "    -    ".join( [original, original_to_converter + " (" + hex(md) + ") " + str( md_check(command4) ), converter_to_host, from_converter_str] ) )

