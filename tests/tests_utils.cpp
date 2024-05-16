#include "tests.hpp"

TEST(Utils, DnsLookupLocalhost)
{
    char *ip = dns_lookup("localhost");
    ASSERT_STREQ(ip, "127.0.0.1");
}

TEST(Utils, DnsLookupLocalhostIP)
{
    char *ip = dns_lookup("127.0.0.1");
    ASSERT_STREQ(ip, "127.0.0.1");
}

TEST(Utils, ReverseDnsLookupLocalhost)
{
    char *host = reverse_dns_lookup("127.0.0.1");
    ASSERT_STREQ(host, "localhost");
}

TEST(Utils, ParseCountArgsNormal)
{
    t_ping_options ping_options;

    const char *values[] = {"5", NULL};
    t_argr argr = {
        .values = (char **)values,
        .option = NULL};
    ASSERT_EQ(parse_count_arg(&ping_options, &argr, "ft_ping"), 0);
    ASSERT_EQ(ping_options.count, 5);
}

TEST(Utils, ParseCountArgsInvalid)
{
    t_ping_options ping_options;

    const char *values[] = {"invalid", NULL};
    t_argr argr = {
        .values = (char **)values,
        .option = NULL};
    ::testing::internal::CaptureStdout();
    ASSERT_NE(parse_count_arg(&ping_options, &argr, "ft_ping"), 0);
    std::string output = ::testing::internal::GetCapturedStdout();
    ASSERT_STREQ(output.c_str(), "ft_ping: invalid count: 'invalid'\n");
}

TEST(Utils, ParseCountArgsOutOfRange)
{
    t_ping_options ping_options;

    const char *values[] = {"9223372036854775808", NULL};
    t_argr argr = {
        .values = (char **)values,
        .option = NULL};
    ::testing::internal::CaptureStdout();
    ASSERT_NE(parse_count_arg(&ping_options, &argr, "ft_ping"), 0);
    std::string output = ::testing::internal::GetCapturedStdout();
    ASSERT_STREQ(output.c_str(), "ft_ping: invalid argument: '9223372036854775808': Numerical result out of range\n");
}
