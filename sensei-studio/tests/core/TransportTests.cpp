#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Transport.hpp"

#include <limits>

using sensei::core::Transport;

TEST_CASE("Transport defaults", "[transport]")
{
    Transport transport;
    REQUIRE_FALSE(transport.isPlaying());
    REQUIRE(transport.bpm() == Catch::Approx(94.0));
    REQUIRE(transport.positionSeconds() == Catch::Approx(0.0));
    REQUIRE(transport.positionBeats() == Catch::Approx(0.0));
}

TEST_CASE("Play and stop reset position", "[transport]")
{
    Transport transport;
    transport.play();
    REQUIRE(transport.isPlaying());

    transport.advance(1.5);
    REQUIRE(transport.positionSeconds() == Catch::Approx(1.5));

    transport.stop();
    REQUIRE_FALSE(transport.isPlaying());
    REQUIRE(transport.positionSeconds() == Catch::Approx(0.0));
}

TEST_CASE("BPM is clamped", "[transport]")
{
    Transport transport;

    transport.setBpm(10.0);
    REQUIRE(transport.bpm() == Catch::Approx(40.0));

    transport.setBpm(400.0);
    REQUIRE(transport.bpm() == Catch::Approx(240.0));

    transport.setBpm(120.0);
    REQUIRE(transport.bpm() == Catch::Approx(120.0));

    transport.setBpm(std::numeric_limits<double>::quiet_NaN());
    REQUIRE(transport.bpm() == Catch::Approx(94.0));
}

TEST_CASE("Advance only while playing", "[transport]")
{
    Transport transport;
    transport.advance(2.0);
    REQUIRE(transport.positionSeconds() == Catch::Approx(0.0));

    transport.play();
    transport.advance(0.5);
    transport.advance(-1.0);
    REQUIRE(transport.positionSeconds() == Catch::Approx(0.5));
}

TEST_CASE("Position beats follows BPM", "[transport]")
{
    Transport transport;
    transport.setBpm(120.0);
    transport.play();
    transport.advance(1.0); // 120 BPM => 2 beats per second
    REQUIRE(transport.positionBeats() == Catch::Approx(2.0));
}

TEST_CASE("resetPosition clears seconds without changing playing state", "[transport]")
{
    Transport transport;
    transport.play();
    transport.advance(3.0);
    transport.resetPosition();
    REQUIRE(transport.isPlaying());
    REQUIRE(transport.positionSeconds() == Catch::Approx(0.0));
}
