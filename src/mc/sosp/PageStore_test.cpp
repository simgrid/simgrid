/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/include/catch.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>

#include <sys/mman.h>
#include <unistd.h>

#include <memory>

#include "src/mc/sosp/PageStore.hpp"

using simgrid::mc::PageStore;

/***********************************/
// a class to hold the variable used in the test cases
class helper_tests {
public:
  static std::size_t pagesize;
  static std::unique_ptr<PageStore> store;
  static void* data;
  static size_t pageno[4];
  static int value;

  // member functions used by the test suite(s)
  static void Init();
  static void store_page_once();
  static void store_same_page();
  static void store_new_page();
  static void unref_pages();
  static void reallocate_page();

  static void new_content(void* buf, std::size_t size);
};

// static member data initialization
std::size_t helper_tests::pagesize             = 0;
std::unique_ptr<PageStore> helper_tests::store = nullptr;
void* helper_tests::data                       = nullptr;
size_t helper_tests::pageno[4]                 = {0, 0, 0, 0};
int helper_tests::value                        = 0;

void helper_tests::Init()
{
  pagesize = (size_t)getpagesize();
  store    = std::make_unique<simgrid::mc::PageStore>(50);
  data     = mmap(nullptr, getpagesize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  REQUIRE(store->size() == 0);
}

void helper_tests::store_page_once()
{
  new_content(data, pagesize);
  pageno[0] = store->store_page(data);
  REQUIRE(store->get_ref(pageno[0]) == 1);
  const void* copy = store->get_page(pageno[0]);
  REQUIRE(::memcmp(data, copy, pagesize) == 0); // The page data should be the same
  REQUIRE(store->size() == 1);
}

void helper_tests::store_same_page()
{
  pageno[1] = store->store_page(data);
  REQUIRE(pageno[0] == pageno[1]); // Page should be the same
  REQUIRE(store->get_ref(pageno[0]) == 2);
  REQUIRE(store->size() == 1);
}

void helper_tests::store_new_page()
{
  new_content(data, pagesize);
  pageno[2] = store->store_page(data);
  REQUIRE(pageno[0] != pageno[2]); // The new page should be different
  REQUIRE(store->size() == 2);
}

void helper_tests::unref_pages()
{
  store->unref_page(pageno[0]);
  REQUIRE(store->get_ref(pageno[0]) == 1);
  REQUIRE(store->size() == 2);

  store->unref_page(pageno[1]);
  REQUIRE(store->size() == 1);
}

void helper_tests::reallocate_page()
{
  new_content(data, pagesize);
  pageno[3] = store->store_page(data);
  REQUIRE(pageno[0] == pageno[3]); // The old page should be reused
  REQUIRE(store->get_ref(pageno[3]) == 1);
  REQUIRE(store->size() == 2);
}

void helper_tests::new_content(void* buf, std::size_t size)
{
  value++;
  ::memset(buf, value, size);
}

TEST_CASE("MC page store, used during checkpoint", "MC::PageStore")
{
  helper_tests::Init();
  INFO("Store page once");
  helper_tests::store_page_once();

  INFO("Store the same page");
  helper_tests::store_same_page();

  INFO("Store a new page");
  helper_tests::store_new_page();

  INFO("Unref pages");
  helper_tests::unref_pages();

  INFO("Reallocate pages");
  helper_tests::reallocate_page();
}
