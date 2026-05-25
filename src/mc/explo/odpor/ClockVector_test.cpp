/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/api/ClockVector.hpp"

using namespace simgrid::mc;

TEST_CASE("simgrid::mc::ClockVector: Constructing Vectors")
{
  SECTION("Without values")
  {
    ClockVector cv;

    // Verify `cv` doesn't map any values
    REQUIRE(not cv.get(0u).has_value());
    REQUIRE(not cv.get(1u).has_value());
    REQUIRE(not cv.get(2u).has_value());
    REQUIRE(not cv.get(3u).has_value());
  }

  SECTION("With initial values")
  {
    ClockVector cv;
    cv[1u]  = 5;
    cv[3u]  = 1;
    cv[7u]  = 10;
    cv[6u]  = 5;
    cv[8u]  = 1;
    cv[10u] = 10;

    // Verify `cv` maps each value
    REQUIRE(cv.get(1u).has_value());
    REQUIRE(cv.get(1u).value() == 5);
    REQUIRE(cv[1u] == 5);
    REQUIRE(cv.get(3u).has_value());
    REQUIRE(cv.get(3u).value() == 1);
    REQUIRE(cv[3u] == 1);
    REQUIRE(cv.get(7u).has_value());
    REQUIRE(cv.get(7u).value() == 10);
    REQUIRE(cv[7u] == 10);
    REQUIRE(cv.get(6u).has_value());
    REQUIRE(cv.get(6u).value() == 5);
    REQUIRE(cv[6u] == 5);
    REQUIRE(cv.get(8u).has_value());
    REQUIRE(cv.get(8u).value() == 1);
    REQUIRE(cv[8u] == 1);
    REQUIRE(cv.get(10u).has_value());
    REQUIRE(cv.get(10u).value() == 10);
    REQUIRE(cv[10u] == 10);
  }
}

TEST_CASE("simgrid::mc::ClockVector: Testing operator[]")
{
  ClockVector cv;
  cv[0u] = 1;

  REQUIRE(cv.get(0u).has_value());
  REQUIRE(cv.get(0u).value() == 1);

  // Verify `cv` doesn't map other values
  REQUIRE(not cv.get(2u).has_value());
  REQUIRE(not cv.get(3u).has_value());

  cv[10u] = 31;

  // Old values are still mapped
  REQUIRE(cv.get(0u).has_value());
  REQUIRE(cv.get(0u).value() == 1);
  REQUIRE(cv[0u] == 1);
  REQUIRE(cv.get(10u).has_value());
  REQUIRE(cv.get(10u).value() == 31);
  REQUIRE(cv[10u] == 31);

  // Verify `cv` doesn't map other values
  REQUIRE_FALSE(cv.get(11u).has_value());
  REQUIRE_FALSE(cv.get(12u).has_value());
}

TEST_CASE("simgrid::mc::ClockVector: Testing Maximal Clock Vectors")
{
  SECTION("Max with zero clock vector yields self")
  {
    ClockVector cv1;
    cv1[1u] = 2;
    cv1[2u] = 10;
    cv1[3u] = 5;

    ClockVector cv2;
    ClockVector maxCV = ClockVector::max(cv1, cv2);

    REQUIRE(maxCV.get(1u).has_value());
    REQUIRE(maxCV.get(1u).value() == 2);
    REQUIRE(maxCV[1u] == 2);

    REQUIRE(maxCV.get(2u).has_value());
    REQUIRE(maxCV.get(2u).value() == 10);
    REQUIRE(maxCV[2u] == 10);

    REQUIRE(maxCV.get(3u).has_value());
    REQUIRE(maxCV.get(3u).value() == 5);
    REQUIRE(maxCV[3u] == 5);
  }

  SECTION("Max with self clock vector yields self")
  {
    ClockVector cv1;
    cv1[1u]           = 2;
    cv1[2u]           = 10;
    cv1[3u]           = 5;
    ClockVector maxCV = ClockVector::max(cv1, cv1);

    REQUIRE(maxCV.get(1u).has_value());
    REQUIRE(maxCV.get(1u).value() == 2);
    REQUIRE(maxCV[1u] == 2);

    REQUIRE(maxCV.get(2u).has_value());
    REQUIRE(maxCV.get(2u).value() == 10);
    REQUIRE(maxCV[2u] == 10);

    REQUIRE(maxCV.get(3u).has_value());
    REQUIRE(maxCV.get(3u).value() == 5);
    REQUIRE(maxCV[3u] == 5);
  }

  SECTION("Testing with partial overlaps")
  {
    SECTION("Example 1")
    {
      ClockVector cv1;
      cv1[1u] = 2;
      cv1[2u] = 10;
      cv1[3u] = 5;
      ClockVector cv2;
      cv2[1u]           = 5;
      cv2[3u]           = 1;
      cv2[7u]           = 10;
      ClockVector maxCV = ClockVector::max(cv1, cv2);

      REQUIRE(maxCV.get(1u).has_value());
      REQUIRE(maxCV.get(1u).value() == 5);
      REQUIRE(maxCV[1u] == 5);

      REQUIRE(maxCV.get(2u).has_value());
      REQUIRE(maxCV.get(2u).value() == 10);
      REQUIRE(maxCV[2u] == 10);

      REQUIRE(maxCV.get(3u).has_value());
      REQUIRE(maxCV.get(3u).value() == 5);
      REQUIRE(maxCV[3u] == 5);

      REQUIRE(maxCV.get(7u).has_value());
      REQUIRE(maxCV.get(7u).value() == 10);
      REQUIRE(maxCV[7u] == 10);
    }

    SECTION("Example 2")
    {
      ClockVector cv1;
      cv1[1u]  = 2;
      cv1[2u]  = 10;
      cv1[3u]  = 5;
      cv1[4u]  = 40;
      cv1[11u] = 3;
      cv1[12u] = 8;

      ClockVector cv2;
      cv2[1u]  = 18;
      cv2[2u]  = 4;
      cv2[4u]  = 41;
      cv2[10u] = 3;
      cv2[12u] = 8;

      ClockVector maxCV = ClockVector::max(cv1, cv2);

      REQUIRE(maxCV.get(1u).has_value());
      REQUIRE(maxCV.get(1u).value() == 18);
      REQUIRE(maxCV[1u] == 18);

      REQUIRE(maxCV.get(2u).has_value());
      REQUIRE(maxCV.get(2u).value() == 10);
      REQUIRE(maxCV[2u] == 10);

      REQUIRE(maxCV.get(3u).has_value());
      REQUIRE(maxCV.get(3u).value() == 5);
      REQUIRE(maxCV[3u] == 5);

      REQUIRE(maxCV.get(4u).has_value());
      REQUIRE(maxCV.get(4u).value() == 41);
      REQUIRE(maxCV[4u] == 41);

      REQUIRE(maxCV.get(10u).has_value());
      REQUIRE(maxCV.get(10u).value() == 3);
      REQUIRE(maxCV[10u] == 3);

      REQUIRE(maxCV.get(11u).has_value());
      REQUIRE(maxCV.get(11u).value() == 3);
      REQUIRE(maxCV[11u] == 3);

      REQUIRE(maxCV.get(12u).has_value());
      REQUIRE(maxCV.get(12u).value() == 8);
      REQUIRE(maxCV[12u] == 8);
    }

    SECTION("Example 3")
    {
      ClockVector cv1;
      cv1[1u]  = 2;
      cv1[4u]  = 41;
      cv1[12u] = 0;
      ClockVector cv2;
      cv2[2u]  = 4;
      cv2[4u]  = 10;
      cv2[10u] = 3;
      cv2[12u] = 8;
      ClockVector cv3;
      cv3[5u]  = 60;
      cv3[12u] = 6;
      cv3[14u] = 3;

      ClockVector maxCV = ClockVector::max(cv1, cv2);
      maxCV             = ClockVector::max(maxCV, cv3);

      REQUIRE(maxCV.get(1u).has_value());
      REQUIRE(maxCV.get(1u).value() == 2);
      REQUIRE(maxCV[1u] == 2);

      REQUIRE(maxCV.get(2u).has_value());
      REQUIRE(maxCV.get(2u).value() == 4);
      REQUIRE(maxCV[2u] == 4);

      REQUIRE(maxCV.get(4u).has_value());
      REQUIRE(maxCV.get(4u).value() == 41);
      REQUIRE(maxCV[4u] == 41);

      REQUIRE(maxCV.get(10u).has_value());
      REQUIRE(maxCV.get(10u).value() == 3);
      REQUIRE(maxCV[10u] == 3);

      REQUIRE(maxCV.get(12u).has_value());
      REQUIRE(maxCV.get(12u).value() == 8);
      REQUIRE(maxCV[12u] == 8);
    }
  }

  SECTION("Testing without overlaps")
  {
    SECTION("Example 1")
    {
      ClockVector cv1;
      cv1[1u] = 2;
      ClockVector cv2;
      cv2[2u]  = 4;
      cv2[4u]  = 41;
      cv2[10u] = 3;
      cv2[12u] = 8;

      ClockVector maxCV = ClockVector::max(cv1, cv2);

      REQUIRE(maxCV.get(1u).has_value());
      REQUIRE(maxCV.get(1u).value() == 2);
      REQUIRE(maxCV[1u] == 2);

      REQUIRE(maxCV.get(2u).has_value());
      REQUIRE(maxCV.get(2u).value() == 4);
      REQUIRE(maxCV[2u] == 4);

      REQUIRE(maxCV.get(4u).has_value());
      REQUIRE(maxCV.get(4u).value() == 41);
      REQUIRE(maxCV[4u] == 41);

      REQUIRE(maxCV.get(10u).has_value());
      REQUIRE(maxCV.get(10u).value() == 3);
      REQUIRE(maxCV[10u] == 3);

      REQUIRE(maxCV.get(12u).has_value());
      REQUIRE(maxCV.get(12u).value() == 8);
      REQUIRE(maxCV[12u] == 8);
    }

    SECTION("Example 2")
    {
      ClockVector cv1;
      cv1[1u] = 2;
      cv1[4u] = 41;
      ClockVector cv2;
      cv2[2u]  = 4;
      cv2[10u] = 3;
      cv2[12u] = 8;

      ClockVector maxCV = ClockVector::max(cv1, cv2);

      REQUIRE(maxCV.get(1u).has_value());
      REQUIRE(maxCV.get(1u).value() == 2);
      REQUIRE(maxCV[1u] == 2);

      REQUIRE(maxCV.get(2u).has_value());
      REQUIRE(maxCV.get(2u).value() == 4);
      REQUIRE(maxCV[2u] == 4);

      REQUIRE(maxCV.get(4u).has_value());
      REQUIRE(maxCV.get(4u).value() == 41);
      REQUIRE(maxCV[4u] == 41);

      REQUIRE(maxCV.get(10u).has_value());
      REQUIRE(maxCV.get(10u).value() == 3);
      REQUIRE(maxCV[10u] == 3);

      REQUIRE(maxCV.get(12u).has_value());
      REQUIRE(maxCV.get(12u).value() == 8);
      REQUIRE(maxCV[12u] == 8);
    }

    SECTION("Example 3")
    {
      ClockVector cv1;
      cv1[1u] = 2;
      cv1[4u] = 41;
      ClockVector cv2;
      cv2[2u]  = 4;
      cv2[10u] = 3;
      cv2[12u] = 8;
      cv2[11u] = 0;
      ClockVector cv3;
      cv3[8u]           = 6;
      cv3[9u]           = 3;
      ClockVector maxCV = ClockVector::max(cv1, cv2);
      maxCV             = ClockVector::max(maxCV, cv3);

      REQUIRE(maxCV.get(1u).has_value());
      REQUIRE(maxCV.get(1u).value() == 2);
      REQUIRE(maxCV[1u] == 2);

      REQUIRE(maxCV.get(2u).has_value());
      REQUIRE(maxCV.get(2u).value() == 4);
      REQUIRE(maxCV[2u] == 4);

      REQUIRE(maxCV.get(4u).has_value());
      REQUIRE(maxCV.get(4u).value() == 41);
      REQUIRE(maxCV[4u] == 41);

      REQUIRE(maxCV.get(10u).has_value());
      REQUIRE(maxCV.get(10u).value() == 3);
      REQUIRE(maxCV[10u] == 3);

      REQUIRE(maxCV.get(12u).has_value());
      REQUIRE(maxCV.get(12u).value() == 8);
      REQUIRE(maxCV[12u] == 8);
    }
  }
}
