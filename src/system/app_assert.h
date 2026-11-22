#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ASSERT(x) app_assert((x), __FILE__, __LINE__)
void app_assert(bool condition, const char *file, int line);

#ifdef __cplusplus
}
#endif
