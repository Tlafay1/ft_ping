#include "tests.hpp"

TEST(Icmp, OpenSocket)
{
    ASSERT_NE(ping_open_socket("ft_ping"), -1);
}

TEST(Icmp, Init)
{
    PING ping;
    int ret = ping_init(&ping, "ft_ping");
    ASSERT_EQ(ret, 0);
}

// TEST(Icmp, CreatePacket)
// {
//     PING ping;
//     struct icmphdr packet;
//     create_packet(&ping, &packet, sizeof(struct icmphdr));
//     ASSERT_EQ(packet.type, ICMP_ECHO);
//     ASSERT_EQ(packet.code, 0);
//     ASSERT_EQ(packet.un.echo.id, htons(ping.ident));
//     ASSERT_EQ(packet.un.echo.sequence, htons(ping.num_emit));
//     ASSERT_EQ(packet.checksum, icmp_cksum((uint16_t *)&packet, sizeof(struct icmphdr)));
// }

// TEST(Icmp, SendPacket)
// {
//     PING ping;
//     ASSERT_EQ(send_packet(&ping), 1);
// }

// TEST(Icmp, RecvPacket)
// {
//     PING ping;
//     ASSERT_EQ(recv_packet(&ping), 1);
// }

// TEST(Icmp, IcmpCksum)
// {
//     uint16_t data[] = {0x1234, 0x5678, 0x9abc};
//     ASSERT_EQ(icmp_cksum(data, sizeof(data)), 0x5e6a);
// }