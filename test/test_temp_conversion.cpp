#include <unity.h>
#include <temphumidespssl.h>

void test_celcius_to_farenheit_conversion_freezing()
{
    TEST_ASSERT_TRUE(tempCtoF(0.0) == 32.0);
}

void test_celcius_to_farenheit_conversion_boiling()
{
    TEST_ASSERT_TRUE(tempCtoF(100.0) == 212.0);
}

void test_celcius_to_farenheit_conversion_edge()
{
    TEST_ASSERT_TRUE(tempCtoF(-40.0) == -40.0);
}

void setup(void)
{
    // set stuff up here
}

void loop(void)
{   
    // clean stuff up here
    UNITY_BEGIN();

    RUN_TEST(test_celcius_to_farenheit_conversion_freezing);
    RUN_TEST(test_celcius_to_farenheit_conversion_boiling);
    RUN_TEST(test_celcius_to_farenheit_conversion_edge);
    
    UNITY_END();
}
