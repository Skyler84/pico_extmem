#include <cstdint>

typedef void(*profiler)(uint32_t);
class ProfileFunc{
public:
  ProfileFunc(profiler p, const char *desc)
  {
    m_next = s_all;
    m_fn = p;
    m_desc = desc;
    s_all = this; 
  }
  static ProfileFunc *all() { return s_all; }
  ProfileFunc *next() const { return m_next; }
  profiler func() const { return m_fn; }
  const char *desc() const { return m_desc; }
private:
  const char *m_desc;
  profiler m_fn;
  ProfileFunc *m_next;
  static ProfileFunc *s_all;
};
void profile_init(IMemory &mem);
uint32_t profile_cps(profiler profile_fn, uint32_t niters = 10'000);
