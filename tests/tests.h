#ifndef WPN_TESTS_H
#define WPN_TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

#define wtest(_name)                                \
    int wpn_unittest_##_name(void)

#define wtest_begin(_name)                          \
    const char* _tname = #_name;                    \
    int _err, _testno = 0, _nerr = 0;               \
    wpnout("starting wpn_unittest_" #_name "\n")    \

#define wtest_assert_soft(_test, ...)                           \
    if (!(_err = (_test))) {                                    \
        wpnerr("%s, test number %d failed: error %d"            \
               __VA_ARGS__ "\n",                                \
               _tname, _testno, _err);                          \
        _nerr++;                                                \
    } else {                                                    \
        wpnout("%s, test number %d passed\n",                   \
              _tname, _testno);                                 \
    } _testno++                                                 \

#define wtest_end                                                   \
    wpnout("wpn_unittest_%s ended, passed %d/%d tests (%.1f\%)\n",  \
           _tname, _testno-_nerr, _testno,                          \
            (100.f/_testno)*(_testno-_nerr));                       \
    return _nerr

#ifdef __cplusplus
}
#endif
#endif
