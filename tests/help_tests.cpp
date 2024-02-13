#include <gtest/gtest.h>
#include "tests.hpp"

TEST(HelpTest, InvalidOption)
{
    const char *argv[] = { "./ft_ping", "-z", NULL };
    std::string expected = "./ft_ping: invalid option -- 'z'\n"
        "Try './ft_ping --help' for more information\n";
    testing::internal::CaptureStdout();
    ft_ping(2, argv);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(expected, output);
}

TEST(HelpTest, HelpMenu)
{
    const char *argv[] = { "./ft_ping", "--help", NULL };
    std::string expected = "Usage: ./ft_ping [options] <destination>\n\n"
        "  -v, --verbose          verbose output\n"
        "  -?, --help             print help and exit\n"
    testing::internal::CaptureStdout();
    ft_ping(2, argv);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(expected, output);
}

TEST(HelpTest, HelpMenuShort)
{
    const char *argv[] = { "./ft_ping", "-h", NULL };
    std::string expected = "Usage: ./ft_ping [options] <destination>\n\n"
        "  -v, --verbose          verbose output\n"
        "  -?,                    print help and exit\n"
        "  -h,                    display this help and exit\n";
    testing::internal::CaptureStdout();
    ft_ping(2, argv);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(expected, output);
}