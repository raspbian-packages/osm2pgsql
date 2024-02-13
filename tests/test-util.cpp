/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2024 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include <catch.hpp>

#include "util.hpp"

#include <cstring>
#include <limits>
#include <string>
#include <vector>

TEST_CASE("double_to_buffer 0", "[NoDB]")
{
    util::double_to_buffer const buffer{0.0};
    REQUIRE(std::strcmp(buffer.c_str(), "0") == 0);
}

TEST_CASE("double_to_buffer 3.141", "[NoDB]")
{
    util::double_to_buffer const buffer{3.141};
    REQUIRE(std::strcmp(buffer.c_str(), "3.141") == 0);
}

TEST_CASE("human readable time durations", "[NoDB]")
{
    REQUIRE(util::human_readable_duration(0) == "0s");
    REQUIRE(util::human_readable_duration(17) == "17s");
    REQUIRE(util::human_readable_duration(59) == "59s");
    REQUIRE(util::human_readable_duration(60) == "60s (1m 0s)");
    REQUIRE(util::human_readable_duration(66) == "66s (1m 6s)");
    REQUIRE(util::human_readable_duration(247) == "247s (4m 7s)");
    REQUIRE(util::human_readable_duration(3599) == "3599s (59m 59s)");
    REQUIRE(util::human_readable_duration(3600) == "3600s (1h 0m 0s)");
    REQUIRE(util::human_readable_duration(3723) == "3723s (1h 2m 3s)");
    REQUIRE(util::human_readable_duration(152592) == "152592s (42h 23m 12s)");
}

TEST_CASE("find_by_name()", "[NoDB]")
{
    class test_class
    {
    public:
        explicit test_class(std::string n) : m_name(std::move(n)) {}
        std::string name() const noexcept { return m_name; }

    private:
        std::string m_name;
    };

    std::vector<test_class> t;

    REQUIRE(util::find_by_name(t, "") == nullptr);
    REQUIRE(util::find_by_name(t, "foo") == nullptr);
    REQUIRE(util::find_by_name(t, "nothing") == nullptr);

    t.emplace_back("foo");
    t.emplace_back("bar");
    t.emplace_back("baz");

    REQUIRE(util::find_by_name(t, "") == nullptr);
    REQUIRE(util::find_by_name(t, "foo") == t.data());
    REQUIRE(util::find_by_name(t, "bar") == &t[1]);
    REQUIRE(util::find_by_name(t, "baz") == &t[2]);
    REQUIRE(util::find_by_name(t, "nothing") == nullptr);
}

TEST_CASE("Use string_joiner_t with delim only without items", "[NoDB]")
{
    util::string_joiner_t joiner{','};
    REQUIRE(joiner.empty());
    REQUIRE(joiner().empty());
}

TEST_CASE("Use string_joiner_t with all params without items", "[NoDB]")
{
    util::string_joiner_t joiner{',', '"', '(', ')'};
    REQUIRE(joiner.empty());
    REQUIRE(joiner().empty());
}

TEST_CASE("Use string_joiner_t without quote char", "[NoDB]")
{
    util::string_joiner_t joiner{',', '\0', '(', ')'};
    REQUIRE(joiner.empty());
    joiner.add("foo");
    REQUIRE_FALSE(joiner.empty());
    joiner.add("bar");
    REQUIRE_FALSE(joiner.empty());
    REQUIRE(joiner() == "(foo,bar)");
}

TEST_CASE("Use string_joiner_t with quote char", "[NoDB]")
{
    util::string_joiner_t joiner{',', '-', '(', ')'};
    REQUIRE(joiner.empty());
    joiner.add("foo");
    REQUIRE_FALSE(joiner.empty());
    joiner.add("bar");
    REQUIRE_FALSE(joiner.empty());
    REQUIRE(joiner() == "(-foo-,-bar-)");
}

TEST_CASE("string_joiner_t without before/after", "[NoDB]")
{
    util::string_joiner_t joiner{','};
    joiner.add("xxx");
    joiner.add("yyy");
    REQUIRE_FALSE(joiner.empty());
    REQUIRE(joiner() == "xxx,yyy");
}

TEST_CASE("string_joiner_t with single single-char item", "[NoDB]")
{
    util::string_joiner_t joiner{','};
    joiner.add("x");
    REQUIRE(joiner() == "x");
}

TEST_CASE("string_joiner_t with single single-char item and wrapper", "[NoDB]")
{
    util::string_joiner_t joiner{',', '\0', '(', ')'};
    joiner.add("x");
    REQUIRE(joiner() == "(x)");
}

TEST_CASE("join strings", "[NoDB]")
{
    std::vector<std::string> const t{"abc", "def", "", "ghi"};

    REQUIRE(util::join(t, ',') == "abc,def,,ghi");
    REQUIRE(util::join(t, '-', '#', '[', ']') == "[#abc#-#def#-##-#ghi#]");
    REQUIRE(util::join(t, '-', '#', '[', ']') == "[#abc#-#def#-##-#ghi#]");
}

TEST_CASE("join strings with empty list", "[NoDB]")
{
    std::vector<std::string> const t{};

    REQUIRE(util::join(t, ',').empty());
    REQUIRE(util::join(t, '-', '#', '[', ']').empty());
}
