#include "boot_animation.h"
#include <cassert>
int main() {
  boot::Animation a;
  a.begin(0xfffffff0u);
  assert(a.oledDue(0xfffffff0u));
  assert(!a.oledDue(0xfffffff1u));
  assert(a.ledDue(0xfffffff0u));
  assert(!a.ledDue(0xfffffff1u));
  assert(!a.update(uint32_t(0xfffffff0u + 1999), false));
  assert(a.update(uint32_t(0xfffffff0u + 2000), false));
  assert(!a.active);
  assert(!a.update(5000, false));
  a.begin(20);
  assert(a.update(21, true));
  assert(!a.active);
  for (unsigned t = 0; t < 2000; t++) {
    unsigned lit = 0;
    for (unsigned i = 0; i < 24; i++) {
      assert(boot::level(t, i, 24, 0) == 0);
      auto n = boot::level(t, i, 24, 10);
      assert(n <= 25);
      assert(n == boot::level(t, i, 24, 100));
      if (n)
        lit++;
    }
    assert(lit == 4);
  }
  for (unsigned h = 0; h < 24; h++) {
    unsigned t = (h * 2000 + 23) / 24;
    assert(boot::level(t, h, 24, 10) == 25);
  }
  assert(boot::level(2000, 0, 24, 10) == 0);
}
