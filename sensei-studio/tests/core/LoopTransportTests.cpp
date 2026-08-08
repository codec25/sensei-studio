#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sensei/core/Transport.hpp"

using namespace sensei::core;

TEST_CASE("Transport wraps inside loop", "[transport][loop]")
{
    Transport transport;
    transport.setBpm(120.0); // 2 beats/sec
    transport.setLoop(0.0, 4.0, true);
    transport.play();

    transport.advance(1.0); // +2 beats
    REQUIRE(transport.positionBeats() == Catch::Approx(2.0));

    transport.advance(1.5); // +3 beats => 5 => wraps to 1
    REQUIRE(transport.positionBeats() == Catch::Approx(1.0));
}

TEST_CASE("Stop resets beats to zero", "[transport][loop]")
{
    Transport transport;
    transport.setLoop(0.0, 16.0, true);
    transport.play();
    transport.advance(0.5);
    transport.stop();
    REQUIRE_FALSE(transport.isPlaying());
    REQUIRE(transport.positionBeats() == Catch::Approx(0.0));
}

TEST_CASE("Existing Milestone A beat/seconds relationship", "[transport]")
{
    Transport transport;
    transport.setBpm(120.0);
    transport.setLoop(0.0, 16.0, false);
    transport.play();
    transport.advance(1.0);
    REQUIRE(transport.positionBeats() == Catch::Approx(2.0));
    REQUIRE(transport.positionSeconds() == Catch::Approx(1.0));
}
