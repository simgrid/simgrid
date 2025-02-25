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
    REQUIRE((not cv.get(0).has_value() or cv.get(0).value() < 0));
    REQUIRE((not cv.get(1).has_value() or cv.get(1).value() < 0));
    REQUIRE((not cv.get(2).has_value() or cv.get(2).value() < 0));
    REQUIRE((not cv.get(3).has_value() or cv.get(3).value() < 0));
  }

  SECTION("With initial values")
  {
    ClockVector cv;
    cv[1]  = 5;
    cv[3]  = 1;
    cv[7]  = 10;
    cv[6]  = 5;
    cv[8]  = 1;
    cv[10] = 10;

    // Verify `cv` maps each value
    REQUIRE(cv.get(1).has_value());
    REQUIRE(cv.get(1).value() == 5);
    REQUIRE(cv[1] == 5);
    REQUIRE(cv.get(3).has_value());
    REQUIRE(cv.get(3).value() == 1);
    REQUIRE(cv[3] == 1);
    REQUIRE(cv.get(7).has_value());
    REQUIRE(cv.get(7).value() == 10);
    REQUIRE(cv[7] == 10);
    REQUIRE(cv.get(6).has_value());
    REQUIRE(cv.get(6).value() == 5);
    REQUIRE(cv[6] == 5);
    REQUIRE(cv.get(8).has_value());
    REQUIRE(cv.get(8).value() == 1);
    REQUIRE(cv[8] == 1);
    REQUIRE(cv.get(10).has_value());
    REQUIRE(cv.get(10).value() == 10);
    REQUIRE(cv[10] == 10);
  }
}

TEST_CASE("simgrid::mc::ClockVector: Testing operator[]")
{
  ClockVector cv;
  cv[0] = 1;

  REQUIRE(cv.get(0).has_value());
  REQUIRE(cv.get(0).value() == 1);

  // Verify `cv` doesn't map other values
  REQUIRE((not cv.get(2).has_value() or cv.get(2).value() < 0));
  REQUIRE((not cv.get(3).has_value() or cv.get(3).value() < 0));

  cv[10] = 31;

  // Old values are still mapped
  REQUIRE(cv.get(0).has_value());
  REQUIRE(cv.get(0).value() == 1);
  REQUIRE(cv[0] == 1);
  REQUIRE(cv.get(10).has_value());
  REQUIRE(cv.get(10).value() == 31);
  REQUIRE(cv[10] == 31);

  // Verify `cv` doesn't map other values
  REQUIRE_FALSE(cv.get(11).has_value());
  REQUIRE_FALSE(cv.get(12).has_value());
}

TEST_CASE("simgrid::mc::ClockVector: Testing Maximal Clock Vectors")
{
  SECTION("Max with zero clock vector yields self")
  {
    ClockVector cv1;
    cv1[1] = 2;
    cv1[2] = 10;
    cv1[3] = 5;

    ClockVector cv2;
    ClockVector maxCV = ClockVector::max(cv1, cv2);

    REQUIRE(maxCV.get(1).has_value());
    REQUIRE(maxCV.get(1).value() == 2);
    REQUIRE(maxCV[1] == 2);

    REQUIRE(maxCV.get(2).has_value());
    REQUIRE(maxCV.get(2).value() == 10);
    REQUIRE(maxCV[2] == 10);

    REQUIRE(maxCV.get(3).has_value());
    REQUIRE(maxCV.get(3).value() == 5);
    REQUIRE(maxCV[3] == 5);
  }

  SECTION("Max with self clock vector yields self")
  {
    ClockVector cv1;
    cv1[1]            = 2;
    cv1[2]            = 10;
    cv1[3]            = 5;
    ClockVector maxCV = ClockVector::max(cv1, cv1);

    REQUIRE(maxCV.get(1).has_value());
    REQUIRE(maxCV.get(1).value() == 2);
    REQUIRE(maxCV[1] == 2);

    REQUIRE(maxCV.get(2).has_value());
    REQUIRE(maxCV.get(2).value() == 10);
    REQUIRE(maxCV[2] == 10);

    REQUIRE(maxCV.get(3).has_value());
    REQUIRE(maxCV.get(3).value() == 5);
    REQUIRE(maxCV[3] == 5);
  }

  SECTION("Testing with partial overlaps")
  {
    SECTION("Example 1")
    {
      ClockVector cv1;
      cv1[1] = 2;
      cv1[2] = 10;
      cv1[3] = 5;
      ClockVector cv2;
      cv2[1]            = 5;
      cv2[3]            = 1;
      cv2[7]            = 10;
      ClockVector maxCV = ClockVector::max(cv1, cv2);

      REQUIRE(maxCV.get(1).has_value());
      REQUIRE(maxCV.get(1).value() == 5);
      REQUIRE(maxCV[1] == 5);

      REQUIRE(maxCV.get(2).has_value());
      REQUIRE(maxCV.get(2).value() == 10);
      REQUIRE(maxCV[2] == 10);

      REQUIRE(maxCV.get(3).has_value());
      REQUIRE(maxCV.get(3).value() == 5);
      REQUIRE(maxCV[3] == 5);

      REQUIRE(maxCV.get(7).has_value());
      REQUIRE(maxCV.get(7).value() == 10);
      REQUIRE(maxCV[7] == 10);
    }

    SECTION("Example 2")
    {
      ClockVector cv1;
      cv1[1]  = 2;
      cv1[2]  = 10;
      cv1[3]  = 5;
      cv1[4]  = 40;
      cv1[11] = 3;
      cv1[12] = 8;

      ClockVector cv2;
      cv2[1]  = 18;
      cv2[2]  = 4;
      cv2[4]  = 41;
      cv2[10] = 3;
      cv2[12] = 8;

      ClockVector maxCV = ClockVector::max(cv1, cv2);

      REQUIRE(maxCV.get(1).has_value());
      REQUIRE(maxCV.get(1).value() == 18);
      REQUIRE(maxCV[1] == 18);

      REQUIRE(maxCV.get(2).has_value());
      REQUIRE(maxCV.get(2).value() == 10);
      REQUIRE(maxCV[2] == 10);

      REQUIRE(maxCV.get(3).has_value());
      REQUIRE(maxCV.get(3).value() == 5);
      REQUIRE(maxCV[3] == 5);

      REQUIRE(maxCV.get(4).has_value());
      REQUIRE(maxCV.get(4).value() == 41);
      REQUIRE(maxCV[4] == 41);

      REQUIRE(maxCV.get(10).has_value());
      REQUIRE(maxCV.get(10).value() == 3);
      REQUIRE(maxCV[10] == 3);

      REQUIRE(maxCV.get(11).has_value());
      REQUIRE(maxCV.get(11).value() == 3);
      REQUIRE(maxCV[11] == 3);

      REQUIRE(maxCV.get(12).has_value());
      REQUIRE(maxCV.get(12).value() == 8);
      REQUIRE(maxCV[12] == 8);
    }

    SECTION("Example 3")
    {
      ClockVector cv1;
      cv1[1]   = 2;
      cv1[4]   = 41;
      cv1[12]  = 0;
      cv1[100] = 5;
      ClockVector cv2;
      cv2[2]  = 4;
      cv2[4]  = 10;
      cv2[10] = 3;
      cv2[12] = 8;
      cv2[19] = 0;
      cv2[21] = 6;
      cv2[22] = 0;
      ClockVector cv3;
      cv3[21]  = 60;
      cv3[22]  = 6;
      cv3[100] = 3;

      ClockVector maxCV = ClockVector::max(cv1, cv2);
      maxCV             = ClockVector::max(maxCV, cv3);

      REQUIRE(maxCV.get(1).has_value());
      REQUIRE(maxCV.get(1).value() == 2);
      REQUIRE(maxCV[1] == 2);

      REQUIRE(maxCV.get(2).has_value());
      REQUIRE(maxCV.get(2).value() == 4);
      REQUIRE(maxCV[2] == 4);

      REQUIRE(maxCV.get(4).has_value());
      REQUIRE(maxCV.get(4).value() == 41);
      REQUIRE(maxCV[4] == 41);

      REQUIRE(maxCV.get(10).has_value());
      REQUIRE(maxCV.get(10).value() == 3);
      REQUIRE(maxCV[10] == 3);

      REQUIRE(maxCV.get(12).has_value());
      REQUIRE(maxCV.get(12).value() == 8);
      REQUIRE(maxCV[12] == 8);

      REQUIRE(maxCV.get(19).has_value());
      REQUIRE(maxCV.get(19).value() == 0);
      REQUIRE(maxCV[19] == 0);

      REQUIRE(maxCV.get(21).has_value());
      REQUIRE(maxCV.get(21).value() == 60);
      REQUIRE(maxCV[21] == 60);

      REQUIRE(maxCV.get(22).has_value());
      REQUIRE(maxCV.get(22).value() == 6);
      REQUIRE(maxCV[22] == 6);

      REQUIRE(maxCV.get(100).has_value());
      REQUIRE(maxCV.get(100).value() == 5);
      REQUIRE(maxCV[100] == 5);
    }
  }

  SECTION("Testing without overlaps")
  {
    SECTION("Example 1")
    {
      ClockVector cv1;
      cv1[1] = 2;
      ClockVector cv2;
      cv2[2]  = 4;
      cv2[4]  = 41;
      cv2[10] = 3;
      cv2[12] = 8;

      ClockVector maxCV = ClockVector::max(cv1, cv2);

      REQUIRE(maxCV.get(1).has_value());
      REQUIRE(maxCV.get(1).value() == 2);
      REQUIRE(maxCV[1] == 2);

      REQUIRE(maxCV.get(2).has_value());
      REQUIRE(maxCV.get(2).value() == 4);
      REQUIRE(maxCV[2] == 4);

      REQUIRE(maxCV.get(4).has_value());
      REQUIRE(maxCV.get(4).value() == 41);
      REQUIRE(maxCV[4] == 41);

      REQUIRE(maxCV.get(10).has_value());
      REQUIRE(maxCV.get(10).value() == 3);
      REQUIRE(maxCV[10] == 3);

      REQUIRE(maxCV.get(12).has_value());
      REQUIRE(maxCV.get(12).value() == 8);
      REQUIRE(maxCV[12] == 8);
    }

    SECTION("Example 2")
    {
      ClockVector cv1;
      cv1[1] = 2;
      cv1[4] = 41;
      ClockVector cv2;
      cv2[2]  = 4;
      cv2[10] = 3;
      cv2[12] = 8;

      ClockVector maxCV = ClockVector::max(cv1, cv2);

      REQUIRE(maxCV.get(1).has_value());
      REQUIRE(maxCV.get(1).value() == 2);
      REQUIRE(maxCV[1] == 2);

      REQUIRE(maxCV.get(2).has_value());
      REQUIRE(maxCV.get(2).value() == 4);
      REQUIRE(maxCV[2] == 4);

      REQUIRE(maxCV.get(4).has_value());
      REQUIRE(maxCV.get(4).value() == 41);
      REQUIRE(maxCV[4] == 41);

      REQUIRE(maxCV.get(10).has_value());
      REQUIRE(maxCV.get(10).value() == 3);
      REQUIRE(maxCV[10] == 3);

      REQUIRE(maxCV.get(12).has_value());
      REQUIRE(maxCV.get(12).value() == 8);
      REQUIRE(maxCV[12] == 8);
    }

    SECTION("Example 3")
    {
      ClockVector cv1;
      cv1[1] = 2;
      cv1[4] = 41;
      ClockVector cv2;
      cv2[2]  = 4;
      cv2[10] = 3;
      cv2[12] = 8;
      cv2[19] = 0;
      cv2[21] = 6;
      ClockVector cv3;
      cv3[22]           = 6;
      cv3[100]          = 3;
      ClockVector maxCV = ClockVector::max(cv1, cv2);
      maxCV             = ClockVector::max(maxCV, cv3);

      REQUIRE(maxCV.get(1).has_value());
      REQUIRE(maxCV.get(1).value() == 2);
      REQUIRE(maxCV[1] == 2);

      REQUIRE(maxCV.get(2).has_value());
      REQUIRE(maxCV.get(2).value() == 4);
      REQUIRE(maxCV[2] == 4);

      REQUIRE(maxCV.get(4).has_value());
      REQUIRE(maxCV.get(4).value() == 41);
      REQUIRE(maxCV[4] == 41);

      REQUIRE(maxCV.get(10).has_value());
      REQUIRE(maxCV.get(10).value() == 3);
      REQUIRE(maxCV[10] == 3);

      REQUIRE(maxCV.get(12).has_value());
      REQUIRE(maxCV.get(12).value() == 8);
      REQUIRE(maxCV[12] == 8);

      REQUIRE(maxCV.get(19).has_value());
      REQUIRE(maxCV.get(19).value() == 0);
      REQUIRE(maxCV[19] == 0);

      REQUIRE(maxCV.get(21).has_value());
      REQUIRE(maxCV.get(21).value() == 6);
      REQUIRE(maxCV[21] == 6);

      REQUIRE(maxCV.get(22).has_value());
      REQUIRE(maxCV.get(22).value() == 6);
      REQUIRE(maxCV[22] == 6);

      REQUIRE(maxCV.get(100).has_value());
      REQUIRE(maxCV.get(100).value() == 3);
      REQUIRE(maxCV[100] == 3);
    }
  }
}
