/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>

#include <sys/mman.h>
#include <unistd.h>

#include <memory>

#include "src/mc/sosp/PageStore.hpp"

/***********************************/
// a class to hold the variable used in the test cases
class pstore_test_helper {
  const size_t pagesize = getpagesize();
  simgrid::mc::PageStore store{50};
  std::byte* data =
      static_cast<std::byte*>(mmap(nullptr, pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  std::array<size_t, 4> pageno = {0, 0, 0, 0};
  int value                    = 0;

  void new_content(std::byte* buf, size_t size);

public:
  // member functions used by the test suite(s)
  void init();
  void store_page_once();
  void store_same_page();
  void store_new_page();
  void unref_pages();
  void reallocate_page();
};

void pstore_test_helper::init()
{
  REQUIRE(data != nullptr);
  REQUIRE(store.size() == 0);
}

void pstore_test_helper::store_page_once()
{
  new_content(data, pagesize);
  pageno[0] = store.store_page(data);
  REQUIRE(store.get_ref(pageno[0]) == 1);
  const auto* copy = store.get_page(pageno[0]);
  REQUIRE(::memcmp(data, copy, pagesize) == 0); // The page data should be the same
  REQUIRE(store.size() == 1);
}

void pstore_test_helper::store_same_page()
{
  pageno[1] = store.store_page(data);
  REQUIRE(pageno[0] == pageno[1]); // Page should be the same
  REQUIRE(store.get_ref(pageno[0]) == 2);
  REQUIRE(store.size() == 1);
}

void pstore_test_helper::store_new_page()
{
  new_content(data, pagesize);
  pageno[2] = store.store_page(data);
  REQUIRE(pageno[0] != pageno[2]); // The new page should be different
  REQUIRE(store.size() == 2);
}

void pstore_test_helper::unref_pages()
{
  store.unref_page(pageno[0]);
  REQUIRE(store.get_ref(pageno[0]) == 1);
  REQUIRE(store.size() == 2);

  store.unref_page(pageno[1]);
  REQUIRE(store.size() == 1);
}

void pstore_test_helper::reallocate_page()
{
  new_content(data, pagesize);
  pageno[3] = store.store_page(data);
  REQUIRE(pageno[0] == pageno[3]); // The old page should be reused
  REQUIRE(store.get_ref(pageno[3]) == 1);
  REQUIRE(store.size() == 2);
}

void pstore_test_helper::new_content(std::byte* buf, size_t size)
{
  value++;
  std::fill_n(buf, size, static_cast<std::byte>(value));
}

TEST_CASE("MC page store, used during checkpoint", "MC::PageStore")
{
  pstore_test_helper pstore_test;
  pstore_test.init();

  INFO("Store page once");
  pstore_test.store_page_once();

  INFO("Store the same page");
  pstore_test.store_same_page();

  INFO("Store a new page");
  pstore_test.store_new_page();

  INFO("Unref pages");
  pstore_test.unref_pages();

  INFO("Reallocate pages");
  pstore_test.reallocate_page();
}
