#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

int webp_get_info(const uint8_t* data, size_t data_size, int* width, int* height);

int test(int n);

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif