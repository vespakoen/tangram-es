#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"
#include "tangram.h"
#include "tile/labels/label.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define EPSILON 0.00001

glm::vec2 screenSize(500.f, 500.f);

TEST_CASE( "Ensure the transition from wait -> sleep when occlusion happens", "[Core][Label]" ) {
    Label l({screenSize/2.f}, "label", 0, Label::Type::POINT);

    REQUIRE(l.getState() == Label::State::WAIT_OCC);
    l.setOcclusion(true);
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);

    REQUIRE(l.getState() != Label::State::SLEEP);
    REQUIRE(l.getState() == Label::State::WAIT_OCC);
    REQUIRE(l.canOcclude());

    l.setOcclusion(true);
    l.occlusionSolved();
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);

    REQUIRE(l.getState() == Label::State::SLEEP);
    REQUIRE(!l.canOcclude());
}

TEST_CASE( "Ensure the transition from wait -> visible when no occlusion happens", "[Core][Label]" ) {
    Label l({screenSize/2.f}, "label", 0, Label::Type::POINT);

    REQUIRE(l.getState() == Label::State::WAIT_OCC);

    l.setOcclusion(false);
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);

    REQUIRE(l.getState() != Label::State::SLEEP);
    REQUIRE(l.getState() == Label::State::WAIT_OCC);

    l.setOcclusion(false);
    l.occlusionSolved();
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);

    REQUIRE(l.getState() == Label::State::FADING_IN);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 1.f);
    REQUIRE(l.getState() == Label::State::VISIBLE);
    REQUIRE(l.canOcclude());
}

TEST_CASE( "Ensure the end state after occlusion is leep state", "[Core][Label]" ) {
    Label l({screenSize/2.f}, "label", 0, Label::Type::POINT);

    l.setOcclusion(false);
    l.occlusionSolved();
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0.f);

    REQUIRE(l.getState() == Label::State::FADING_IN);
    REQUIRE(l.canOcclude());

    l.setOcclusion(true);
    l.occlusionSolved();
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0.f);

    REQUIRE(l.getState() == Label::State::SLEEP);
    REQUIRE(!l.canOcclude());
}

TEST_CASE( "Ensure the out of screen state transition", "[Core][Label]" ) {
    Label l({screenSize*2.f}, "label", 0, Label::Type::POINT);

    REQUIRE(l.getState() == Label::State::WAIT_OCC);

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0.f);

    REQUIRE(l.getState() == Label::State::OUT_OF_SCREEN);
    REQUIRE(!l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x * 4.f, screenSize.y * 4.f, 0.f, -1.f, 1.f), screenSize, 0.f);
    REQUIRE(l.getState() == Label::State::WAIT_OCC);
    REQUIRE(l.canOcclude());

    l.setOcclusion(false);
    l.occlusionSolved();
    l.update(glm::ortho(0.f, screenSize.x * 4.f, screenSize.y * 4.f, 0.f, -1.f, 1.f), screenSize, 0.f);
    REQUIRE(l.getState() != Label::State::WAIT_OCC);

    REQUIRE(l.getState() == Label::State::FADING_IN);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x * 4.f, screenSize.y * 4.f, 0.f, -1.f, 1.f), screenSize, 1.f);

    REQUIRE(l.getState() == Label::State::VISIBLE);
    REQUIRE(l.canOcclude());
}

TEST_CASE( "Ensure debug labels are always visible and cannot occlude", "[Core][Label]" ) {
    Label l({screenSize/2.f}, "label", 0, Label::Type::DEBUG);

    REQUIRE(l.getState() == Label::State::VISIBLE);
    REQUIRE(!l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 1.f);

    REQUIRE(l.getState() == Label::State::VISIBLE);
    REQUIRE(!l.canOcclude());
}

TEST_CASE( "Linear interpolation", "[Core][Label][Fade]" ) {
    FadeEffect fadeOut(false, FadeEffect::Interpolation::LINEAR, 1.0);

    REQUIRE(fadeOut.update(0.0) == 1.0);
    REQUIRE(fadeOut.update(0.5) == 0.5);
    REQUIRE(fadeOut.update(0.5) == 0.0);

    fadeOut.update(0.01);

    REQUIRE(fadeOut.isFinished());

    FadeEffect fadeIn(true, FadeEffect::Interpolation::LINEAR, 1.0);

    REQUIRE(fadeIn.update(0.0) == 0.0);
    REQUIRE(fadeIn.update(0.5) == 0.5);
    REQUIRE(fadeIn.update(0.5) == 1.0);

    fadeIn.update(0.01);

    REQUIRE(fadeIn.isFinished());
}

TEST_CASE( "Pow interpolation", "[Core][Label][Fade]" ) {
    FadeEffect fadeOut(false, FadeEffect::Interpolation::POW, 1.0);

    REQUIRE(fadeOut.update(0.0) == 1.0);
    REQUIRE(fadeOut.update(0.5) == 0.75);
    REQUIRE(fadeOut.update(0.5) == 0.0);

    fadeOut.update(0.01);

    REQUIRE(fadeOut.isFinished());

    FadeEffect fadeIn(true, FadeEffect::Interpolation::POW, 1.0);

    REQUIRE(fadeIn.update(0.0) == 0.0);
    REQUIRE(fadeIn.update(0.5) == 0.25);
    REQUIRE(fadeIn.update(0.5) == 1.0);

    fadeIn.update(0.01);

    REQUIRE(fadeIn.isFinished());
}

TEST_CASE( "Sine interpolation", "[Core][Label][Fade]" ) {
    FadeEffect fadeOut(false, FadeEffect::Interpolation::SINE, 1.0);

    REQUIRE(std::fabs(fadeOut.update(0.0) - 1.0) < EPSILON);
    REQUIRE(std::fabs(fadeOut.update(1.0) - 0.0) < EPSILON);

    fadeOut.update(0.01);

    REQUIRE(fadeOut.isFinished());

    FadeEffect fadeIn(true, FadeEffect::Interpolation::SINE, 1.0);

    REQUIRE(std::fabs(fadeIn.update(0.0) - 0.0) < EPSILON);
    REQUIRE(std::fabs(fadeIn.update(1.0) - 1.0) < EPSILON);

    fadeIn.update(0.01);

    REQUIRE(fadeIn.isFinished());
}

