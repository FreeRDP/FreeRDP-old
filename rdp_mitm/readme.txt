rdp_mitm: A Man-In-The-Middle Proxy for Remote Desktop using TLS and NTLMSSP

Compile using: gcc -o rdp_mitm rdp_mitm.c -lssl

Usage: ./server <listening port> <server address> <server port>

This program is experimental and prone to errors. I've only used it with Windows Server 2008.
In my case, the client and server negotiate usage of NTLMSSP, but it may as well be Kerberos.

At this moment, the messages are decrypted but the server disconnects the client because of
something wrong in NTLMSSP. In the very worst case, implementating full NTLMSSP authentication
between the client and the proxy with prior knowledge of the password should work.

Wireshark:
In Wireshark, go to Edit->Preferences, expand "Protocols" on the left and select "SSL". Set
"RSA Key List" to <proxy IP address>,<proxy port>,<decrypted payload dissector name>,<full path to private key>

Example:
192.168.1.110,1337,data,C:\server.key

Where the proxy is at 192.168.1.110 and listening on port 1337, and the payload is passed to the generic "data"
dissector that in fact doesn't dissect anything. C:\server.key is the path to the key file used by the proxy.

Example usage:

aghaster@debian:~/c/rdp_mitm$ ./rdp_mitm 1337 192.168.1.125 3389
Got connection from 192.168.1.125
Received RDP client initial from source
03 00 00 13 0E E0 00 00 00 00 00 01 00 08 00 03        ................
00 00 00                                               ...
Sent RDP server initial to source
03 00 00 13 0E D0 00 00 12 34 00 02 00 08 00 02        .........4......
00 00 00                                               ...
TLS connection established with source
Connected to 192.168.1.125 on port 3389
Sent RDP client initial to destination
03 00 00 13 0E E0 00 00 00 00 00 01 00 08 00 03        ................
00 00 00                                               ...
Received RDP server initial from destination
03 00 00 13 0E D0 00 00 12 34 00 02 00 08 00 02        .........4......
00 00 00                                               ...
TLS connection established with destination
srcfd readable
read 57 bytes
30 37 A0 03 02 01 02 A1 30 30 2E 30 2C A0 2A 04        07......00.0,.*.
28 4E 54 4C 4D 53 53 50 00 01 00 00 00 B7 82 08        (NTLMSSP........
E2 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00        ................
00 06 00 72 17 00 00 00 0F                             ...r.....
dstfd writable
Wrote 57 bytes
30 37 A0 03 02 01 02 A1 30 30 2E 30 2C A0 2A 04        07......00.0,.*.
28 4E 54 4C 4D 53 53 50 00 01 00 00 00 B7 82 08        (NTLMSSP........
E2 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00        ................
00 06 00 72 17 00 00 00 0F                             ...r.....
dstfd readable
read 181 bytes
30 81 B2 A0 03 02 01 02 A1 81 AA 30 81 A7 30 81        0..........0..0.
A4 A0 81 A1 04 81 9E 4E 54 4C 4D 53 53 50 00 02        .......NTLMSSP..
00 00 00 0E 00 0E 00 38 00 00 00 35 82 8A E2 BF        .......8...5....
E7 2F 48 DA 9A 71 29 00 00 00 00 00 00 00 00 58        ./H..q)........X
00 58 00 46 00 00 00 06 00 72 17 00 00 00 0F 46        .X.F.....r.....F
00 52 00 45 00 45 00 52 00 44 00 50 00 02 00 0E        .R.E.E.R.D.P....
00 46 00 52 00 45 00 45 00 52 00 44 00 50 00 01        .F.R.E.E.R.D.P..
00 0E 00 46 00 52 00 45 00 45 00 52 00 44 00 50        ...F.R.E.E.R.D.P
00 04 00 0E 00 46 00 72 00 65 00 65 00 52 00 44        .....F.r.e.e.R.D
00 50 00 03 00 0E 00 46 00 72 00 65 00 65 00 52        .P.....F.r.e.e.R
00 44 00 50 00 07 00 08 00 E9 AF 2C E0 F0 5C CA        .D.P.......,..\.
01 00 00 00 00                                         .....
srcfd writable
Wrote 181 bytes
30 81 B2 A0 03 02 01 02 A1 81 AA 30 81 A7 30 81        0..........0..0.
A4 A0 81 A1 04 81 9E 4E 54 4C 4D 53 53 50 00 02        .......NTLMSSP..
00 00 00 0E 00 0E 00 38 00 00 00 35 82 8A E2 BF        .......8...5....
E7 2F 48 DA 9A 71 29 00 00 00 00 00 00 00 00 58        ./H..q)........X
00 58 00 46 00 00 00 06 00 72 17 00 00 00 0F 46        .X.F.....r.....F
00 52 00 45 00 45 00 52 00 44 00 50 00 02 00 0E        .R.E.E.R.D.P....
00 46 00 52 00 45 00 45 00 52 00 44 00 50 00 01        .F.R.E.E.R.D.P..
00 0E 00 46 00 52 00 45 00 45 00 52 00 44 00 50        ...F.R.E.E.R.D.P
00 04 00 0E 00 46 00 72 00 65 00 65 00 52 00 44        .....F.r.e.e.R.D
00 50 00 03 00 0E 00 46 00 72 00 65 00 65 00 52        .P.....F.r.e.e.R
00 44 00 50 00 07 00 08 00 E9 AF 2C E0 F0 5C CA        .D.P.......,..\.
01 00 00 00 00                                         .....
srcfd readable
read 695 bytes
30 82 02 B3 A0 03 02 01 02 A1 82 01 84 30 82 01        0............0..
80 30 82 01 7C A0 82 01 78 04 82 01 74 4E 54 4C        .0..|...x...tNTL
4D 53 53 50 00 03 00 00 00 18 00 18 00 84 00 00        MSSP............
00 C8 00 C8 00 9C 00 00 00 0E 00 0E 00 58 00 00        .............X..
00 10 00 10 00 66 00 00 00 0E 00 0E 00 76 00 00        .....f.......v..
00 10 00 10 00 64 01 00 00 35 82 88 E2 06 00 72        .....d...5.....r
17 00 00 00 0F 98 03 1C C0 47 A4 ED 11 4B 7B 16        .........G...K{.
1A F9 45 89 F3 46 00 52 00 45 00 45 00 52 00 44        ..E..F.R.E.E.R.D
00 50 00 75 00 73 00 65 00 72 00 6E 00 61 00 6D        .P.u.s.e.r.n.a.m
00 65 00 46 00 52 00 45 00 45 00 52 00 44 00 50        .e.F.R.E.E.R.D.P
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00        ................
00 00 00 00 00 00 00 00 00 2E B6 9D 10 71 7F 46        .............qF
D4 8B 3F D5 98 28 65 08 B2 01 01 00 00 00 00 00        ..?..(e.........
00 E9 AF 2C E0 F0 5C CA 01 FB 27 54 C3 7D 25 D3        ...,..\...'T.}%.
B2 00 00 00 00 02 00 0E 00 46 00 52 00 45 00 45        .........F.R.E.E
00 52 00 44 00 50 00 01 00 0E 00 46 00 52 00 45        .R.D.P.....F.R.E
00 45 00 52 00 44 00 50 00 04 00 0E 00 46 00 72        .E.R.D.P.....F.r
00 65 00 65 00 52 00 44 00 50 00 03 00 0E 00 46        .e.e.R.D.P.....F
00 72 00 65 00 65 00 52 00 44 00 50 00 07 00 08        .r.e.e.R.D.P....
00 E9 AF 2C E0 F0 5C CA 01 06 00 04 00 02 00 00        ...,..\.........
00 08 00 30 00 30 00 00 00 00 00 00 00 00 00 00        ...0.0..........
00 00 30 00 00 E7 D0 2F 86 0A 2A FD B9 D0 3D BC        ..0..../..*...=.
94 80 BD D9 B4 8D F6 AA ED 77 24 FF 5B 8F 59 2F        .........w$.[.Y/
6C 37 E6 A1 6D 00 00 00 00 00 00 00 00 00 00 00        l7..m...........
00 38 B7 4E 17 B2 9D 6E EF 52 02 53 E9 09 A3 DF        .8.N...n.R.S....
54 A3 82 01 22 04 82 01 1E 01 00 00 00 D6 CE 16        T..."...........
42 9A 6F 40 BA 00 00 00 00 71 84 9C E4 7E 54 AF        B.o@.....q...~T.
D4 59 4C CE BB DE 6E 2A 61 0E 8D 39 5A 3A FF 9C        .YL...n*a..9Z:..
3F E3 49 D8 07 6F 7F 34 AC 9F 01 A4 37 BC 42 21        ?.I..o4....7.B!
B0 65 6D D0 79 BF 69 11 70 DD 7D E8 0D 7F AB D4        .em.y.i.p.}....
7F 35 07 AC E4 05 A1 63 A2 27 13 79 6D 05 EB 5C        5.....c.'.ym..\
9F 67 43 B3 3F 88 71 C7 BA A5 88 34 2F 8B 54 88        .gC.?.q....4/.T.
D5 AF 53 26 09 1F 34 37 BD 4A 41 1D C1 58 31 9F        ..S&..47.JA..X1.
27 EC D4 8E 13 0F 52 85 89 6E 5A DE B8 19 A1 24        '.....R..nZ....$
66 DF BC DA 32 0F 7F FD 9E D5 BE 2C 50 F8 3D AD        f...2.....,P.=.
23 CD 59 1C ED 05 40 35 A3 85 CF 1D 0D 38 B0 41        #.Y...@5.....8.A
1E 21 CF E5 B8 8D 0F A5 4A A0 F3 F5 09 39 04 99        .!......J....9..
3C 64 B9 8F 8A A1 20 A9 1E 6D B7 27 0C 33 DA 07        <d.... ..m.'.3..
11 A4 A6 12 87 BA A4 B0 BE 38 A9 1A 06 DE F5 C5        .........8......
8A A0 5C 34 85 B8 3D ED 9E 75 F3 38 EF DC 80 4D        ..\4..=..u.8...M
2A C3 53 57 D1 90 3E DF 09 A8 69 47 26 57 85 4D        *.SW..>...iG&W.M
51 94 6F A2 5E 6B 25 70 26 40 4D 39 EA CD 0F 47        Q.o.^k%p&@M9...G
17 58 55 4C 0E DE C2 27 DC 03 38 21 21 FD 7E CC        .XUL...'..8!!.~.
C8 82 DE 04 35 74 2D                                   ....5t-
dstfd writable
Wrote 695 bytes
30 82 02 B3 A0 03 02 01 02 A1 82 01 84 30 82 01        0............0..
80 30 82 01 7C A0 82 01 78 04 82 01 74 4E 54 4C        .0..|...x...tNTL
4D 53 53 50 00 03 00 00 00 18 00 18 00 84 00 00        MSSP............
00 C8 00 C8 00 9C 00 00 00 0E 00 0E 00 58 00 00        .............X..
00 10 00 10 00 66 00 00 00 0E 00 0E 00 76 00 00        .....f.......v..
00 10 00 10 00 64 01 00 00 35 82 88 E2 06 00 72        .....d...5.....r
17 00 00 00 0F 98 03 1C C0 47 A4 ED 11 4B 7B 16        .........G...K{.
1A F9 45 89 F3 46 00 52 00 45 00 45 00 52 00 44        ..E..F.R.E.E.R.D
00 50 00 75 00 73 00 65 00 72 00 6E 00 61 00 6D        .P.u.s.e.r.n.a.m
00 65 00 46 00 52 00 45 00 45 00 52 00 44 00 50        .e.F.R.E.E.R.D.P
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00        ................
00 00 00 00 00 00 00 00 00 2E B6 9D 10 71 7F 46        .............qF
D4 8B 3F D5 98 28 65 08 B2 01 01 00 00 00 00 00        ..?..(e.........
00 E9 AF 2C E0 F0 5C CA 01 FB 27 54 C3 7D 25 D3        ...,..\...'T.}%.
B2 00 00 00 00 02 00 0E 00 46 00 52 00 45 00 45        .........F.R.E.E
00 52 00 44 00 50 00 01 00 0E 00 46 00 52 00 45        .R.D.P.....F.R.E
00 45 00 52 00 44 00 50 00 04 00 0E 00 46 00 72        .E.R.D.P.....F.r
00 65 00 65 00 52 00 44 00 50 00 03 00 0E 00 46        .e.e.R.D.P.....F
00 72 00 65 00 65 00 52 00 44 00 50 00 07 00 08        .r.e.e.R.D.P....
00 E9 AF 2C E0 F0 5C CA 01 06 00 04 00 02 00 00        ...,..\.........
00 08 00 30 00 30 00 00 00 00 00 00 00 00 00 00        ...0.0..........
00 00 30 00 00 E7 D0 2F 86 0A 2A FD B9 D0 3D BC        ..0..../..*...=.
94 80 BD D9 B4 8D F6 AA ED 77 24 FF 5B 8F 59 2F        .........w$.[.Y/
6C 37 E6 A1 6D 00 00 00 00 00 00 00 00 00 00 00        l7..m...........
00 38 B7 4E 17 B2 9D 6E EF 52 02 53 E9 09 A3 DF        .8.N...n.R.S....
54 A3 82 01 22 04 82 01 1E 01 00 00 00 D6 CE 16        T..."...........
42 9A 6F 40 BA 00 00 00 00 71 84 9C E4 7E 54 AF        B.o@.....q...~T.
D4 59 4C CE BB DE 6E 2A 61 0E 8D 39 5A 3A FF 9C        .YL...n*a..9Z:..
3F E3 49 D8 07 6F 7F 34 AC 9F 01 A4 37 BC 42 21        ?.I..o4....7.B!
B0 65 6D D0 79 BF 69 11 70 DD 7D E8 0D 7F AB D4        .em.y.i.p.}....
7F 35 07 AC E4 05 A1 63 A2 27 13 79 6D 05 EB 5C        5.....c.'.ym..\
9F 67 43 B3 3F 88 71 C7 BA A5 88 34 2F 8B 54 88        .gC.?.q....4/.T.
D5 AF 53 26 09 1F 34 37 BD 4A 41 1D C1 58 31 9F        ..S&..47.JA..X1.
27 EC D4 8E 13 0F 52 85 89 6E 5A DE B8 19 A1 24        '.....R..nZ....$
66 DF BC DA 32 0F 7F FD 9E D5 BE 2C 50 F8 3D AD        f...2.....,P.=.
23 CD 59 1C ED 05 40 35 A3 85 CF 1D 0D 38 B0 41        #.Y...@5.....8.A
1E 21 CF E5 B8 8D 0F A5 4A A0 F3 F5 09 39 04 99        .!......J....9..
3C 64 B9 8F 8A A1 20 A9 1E 6D B7 27 0C 33 DA 07        <d.... ..m.'.3..
11 A4 A6 12 87 BA A4 B0 BE 38 A9 1A 06 DE F5 C5        .........8......
8A A0 5C 34 85 B8 3D ED 9E 75 F3 38 EF DC 80 4D        ..\4..=..u.8...M
2A C3 53 57 D1 90 3E DF 09 A8 69 47 26 57 85 4D        *.SW..>...iG&W.M
51 94 6F A2 5E 6B 25 70 26 40 4D 39 EA CD 0F 47        Q.o.^k%p&@M9...G
17 58 55 4C 0E DE C2 27 DC 03 38 21 21 FD 7E CC        .XUL...'..8!!.~.
C8 82 DE 04 35 74 2D                                   ....5t-
dstfd readable
SSL_ERROR_SSL
21510:error:14094419:SSL routines:SSL3_READ_BYTES:tlsv1 alert access denied:s3_pkt.c:1060:SSL alert number 49
dstfd readable
Connection with was closed

Here are the three messages above parsed using OpenSSL's ASN.1 parser:

debian:/home/aghaster# openssl asn1parse -in 1.bin -inform der
    0:d=0  hl=2 l=  55 cons: SEQUENCE         
    2:d=1  hl=2 l=   3 cons: cont [ 0 ]       
    4:d=2  hl=2 l=   1 prim: INTEGER           :02
    7:d=1  hl=2 l=  48 cons: cont [ 1 ]       
    9:d=2  hl=2 l=  46 cons: SEQUENCE         
   11:d=3  hl=2 l=  44 cons: SEQUENCE         
   13:d=4  hl=2 l=  42 cons: cont [ 0 ]       
   15:d=5  hl=2 l=  40 prim: OCTET STRING      [HEX DUMP]:4E544C4D5353500001000000B78208E200000000000000000000000000000000060072170000000F
debian:/home/aghaster# openssl asn1parse -in 2.bin -inform der
    0:d=0  hl=3 l= 178 cons: SEQUENCE         
    3:d=1  hl=2 l=   3 cons: cont [ 0 ]       
    5:d=2  hl=2 l=   1 prim: INTEGER           :02
    8:d=1  hl=3 l= 170 cons: cont [ 1 ]       
   11:d=2  hl=3 l= 167 cons: SEQUENCE         
   14:d=3  hl=3 l= 164 cons: SEQUENCE         
   17:d=4  hl=3 l= 161 cons: cont [ 0 ]       
   20:d=5  hl=3 l= 158 prim: OCTET STRING      [HEX DUMP]:4E544C4D53535000020000000E000E003800000035828AE2BFE72F48DA9A712900000000000000005800580046000000060072170000000F460052004500450052004400500002000E00460052004500450052004400500001000E00460052004500450052004400500004000E00460072006500650052004400500003000E00460072006500650052004400500007000800E9AF2CE0F05CCA0100000000
debian:/home/aghaster# openssl asn1parse -in 3.bin -inform der
    0:d=0  hl=4 l= 691 cons: SEQUENCE         
    4:d=1  hl=2 l=   3 cons: cont [ 0 ]       
    6:d=2  hl=2 l=   1 prim: INTEGER           :02
    9:d=1  hl=4 l= 388 cons: cont [ 1 ]       
   13:d=2  hl=4 l= 384 cons: SEQUENCE         
   17:d=3  hl=4 l= 380 cons: SEQUENCE         
   21:d=4  hl=4 l= 376 cons: cont [ 0 ]       
   25:d=5  hl=4 l= 372 prim: OCTET STRING      [HEX DUMP]:4E544C4D53535000030000001800180084000000C800C8009C0000000E000E005800000010001000660000000E000E00760000001000100064010000358288E2060072170000000F98031CC047A4ED114B7B161AF94589F3460052004500450052004400500075007300650072006E0061006D00650046005200450045005200440050000000000000000000000000000000000000000000000000002EB69D10717F46D48B3FD598286508B20101000000000000E9AF2CE0F05CCA01FB2754C37D25D3B20000000002000E00460052004500450052004400500001000E00460052004500450052004400500004000E00460072006500650052004400500003000E00460072006500650052004400500007000800E9AF2CE0F05CCA0106000400020000000800300030000000000000000000000000300000E7D02F860A2AFDB9D03DBC9480BDD9B48DF6AAED7724FF5B8F592F6C37E6A16D00000000000000000000000038B74E17B29D6EEF520253E909A3DF54
  401:d=1  hl=4 l= 290 cons: cont [ 3 ]       
  405:d=2  hl=4 l= 286 prim: OCTET STRING      [HEX DUMP]:01000000D6CE16429A6F40BA0000000071849CE47E54AFD4594CCEBBDE6E2A610E8D395A3AFF9C3FE349D8076F7F34AC9F01A437BC4221B0656DD079BF691170DD7DE80D7FABD47F3507ACE405A163A22713796D05EB5C9F6743B33F8871C7BAA588342F8B5488D5AF5326091F3437BD4A411DC158319F27ECD48E130F5285896E5ADEB819A12466DFBCDA320F7FFD9ED5BE2C50F83DAD23CD591CED054035A385CF1D0D38B0411E21CFE5B88D0FA54AA0F3F5093904993C64B98F8AA120A91E6DB7270C33DA0711A4A61287BAA4B0BE38A91A06DEF5C58AA05C3485B83DED9E75F338EFDC804D2AC35357D1903EDF09A869472657854D51946FA25E6B257026404D39EACD0F471758554C0EDEC227DC03382121FD7ECCC882DE0435742D

