/* Copyright 2026, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of CashBook:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file: test_stringbuild.c
 *
 * Unit Tests for the stringbuild.c code.
 */

/* ANSI C Header files. */

#include <string.h>

/* Acorn C Header files. */

#include "flex.h"

/* SFLib Header files. */

#include "sflib/msgs.h"

/* Unity Header files. */

#include "unity.h"

/* Locate Application header file. */

#include "stringbuild.h"

/**
 * Unit Test setup.
 */

void setUp(void)
{
	msgs_initialise("<CashBook$TestDir>.Messages");
}

/**
 * Unit Test teardown.
 */

void tearDown(void)
{
	msgs_terminate();
}

/**
 * The Unit Tests.
 */

/*** Bad initialisation ***/

void test_initialise_to_null_buffer()
{
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_initialise(NULL, 1), "stringbuild_initialise() returns FALSE on error");
}

void test_initialise_to_null_zero_lenth_buffer()
{
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_initialise(NULL, 0), "stringbuild_initialise() returns FALSE on error");
}

void test_initialise_to_zero_length_buffer()
{
	char buffer[1];

	TEST_ASSERT_FALSE_MESSAGE(stringbuild_initialise(buffer, 0), "stringbuild_initialise() returns FALSE on error");
}

/*** Initialising and reading an empty line ***/

void test_initialise_read_empty_line()
{
	char buffer[100];

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 100), "stringbuild_initialise() returns TRUE");

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("", line, "The returned string is empty");

	/* Allowing for the terminator, there are 99 bytes left from the 100 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(99, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

/*** Writing an string object and reading it back. ***/

void test_initialise_write_and_read_empty_string()
{
	char buffer[100];

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 100), "stringbuild_initialise() returns TRUE");
	stringbuild_add_string("");

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("", line, "The returned string is empty");

	/* Allowing for the terminator, there are still 99 bytes left from the 100 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(99, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_short_string()
{
	char buffer[100];

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 100), "stringbuild_initialise() returns TRUE");
	stringbuild_add_string("1234567890");

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890", line, "The returned string is correct");

	/* Allowing for the terminator, there are 89 bytes left from the 100 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(89, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_full_length_string()
{
	char buffer[12];
	buffer[11] = '~';

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 11), "stringbuild_initialise() returns TRUE");
	stringbuild_add_string("1234567890");

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890", line, "The returned string is correct");

	/* Allowing for the terminator, there are 0 bytes left from the 11 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_EQUAL_CHAR_MESSAGE('~', buffer[11], "The guard character hasn't changed");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_over_length_string()
{
	char buffer[6];
	buffer[5] = '~';

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 5), "stringbuild_initialise() returns TRUE");
	stringbuild_add_string("1234567890");

	char *line = stringbuild_get_line();

	TEST_ASSERT_NULL_MESSAGE(line, "The returned line is NULL");
//	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
//	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234", line, "The returned string is correct");

	/* Allowing for the terminator, there are 0 bytes left from the 11 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_EQUAL_CHAR_MESSAGE('~', buffer[5], "The guard character hasn't changed");
	TEST_ASSERT_TRUE_MESSAGE(stringbuild_get_too_long(), "The too long flag is set");
}

/*** Writing a printf object and reading it back. ***/

void test_initialise_write_and_read_empty_printf()
{
	char buffer[100];

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 100), "stringbuild_initialise() returns TRUE");
	stringbuild_add_printf("");

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("", line, "The returned string is empty");

	/* Allowing for the terminator, there are still 99 bytes left from the 100 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(99, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_short_printf()
{
	char buffer[100];

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 100), "stringbuild_initialise() returns TRUE");
	stringbuild_add_printf("12345%d", 67890);

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890", line, "The returned string is correct");

	/* Allowing for the terminator, there are 89 bytes left from the 100 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(89, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_full_length_printf()
{
	char buffer[12];
	buffer[11] = '~';

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 11), "stringbuild_initialise() returns TRUE");
	stringbuild_add_printf("12345%d", 67890);

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890", line, "The returned string is correct");

	/* Allowing for the terminator, there are 0 bytes left from the 11 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_EQUAL_CHAR_MESSAGE('~', buffer[11], "The guard character hasn't changed");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_over_length_printf()
{
	char buffer[6];
	buffer[5] = '~';

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 5), "stringbuild_initialise() returns TRUE");
	stringbuild_add_printf("12345%d", 67890);

	char *line = stringbuild_get_line();

	TEST_ASSERT_NULL_MESSAGE(line, "The returned line is NULL");
//	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
//	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234", line, "The returned string is correct");

	/* Allowing for the terminator, there are 0 bytes left from the 11 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_EQUAL_CHAR_MESSAGE('~', buffer[5], "The guard character hasn't changed");
	TEST_ASSERT_TRUE_MESSAGE(stringbuild_get_too_long(), "The too long flag is set");
}

/*** Writing a fixed message object and reading it back. ***/

void test_initialise_write_and_read_empty_fixed_msgs()
{
	char buffer[100];

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 100), "stringbuild_initialise() returns TRUE");
	stringbuild_add_message("Empty");

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("", line, "The returned string is empty");

	/* Allowing for the terminator, there are still 99 bytes left from the 100 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(99, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_short_fixed_msgs()
{
	char buffer[100];

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 100), "stringbuild_initialise() returns TRUE");
	stringbuild_add_message("Fixed10");

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890", line, "The returned string is correct");

	/* Allowing for the terminator, there are 89 bytes left from the 100 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(89, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_full_length_fixed_msgs()
{
	char buffer[12];
	buffer[11] = '~';

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 11), "stringbuild_initialise() returns TRUE");
	stringbuild_add_message("Fixed10");

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890", line, "The returned string is correct");

	/* Allowing for the terminator, there are 0 bytes left from the 11 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_EQUAL_CHAR_MESSAGE('~', buffer[11], "The guard character hasn't changed");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_over_length_fixed_msgs()
{
	char buffer[6];
	buffer[5] = '~';

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 5), "stringbuild_initialise() returns TRUE");
	stringbuild_add_message("Fixed10");

	char *line = stringbuild_get_line();

	TEST_ASSERT_NULL_MESSAGE(line, "The returned line is NULL");
//	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
//	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234", line, "The returned string is correct");

	/* Allowing for the terminator, there are 0 bytes left from the 11 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_EQUAL_CHAR_MESSAGE('~', buffer[5], "The guard character hasn't changed");
	TEST_ASSERT_TRUE_MESSAGE(stringbuild_get_too_long(), "The too long flag is set");
}

/*** Writing a parameterised message object and reading it back. ***/

void test_initialise_write_and_read_empty_param_msgs()
{
	char buffer[100];

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 100), "stringbuild_initialise() returns TRUE");
	stringbuild_add_message_param("Empty", NULL, NULL, NULL, NULL);

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("", line, "The returned string is empty");

	/* Allowing for the terminator, there are still 99 bytes left from the 100 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(99, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_short_param_msgs()
{
	char buffer[100];

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 100), "stringbuild_initialise() returns TRUE");
	stringbuild_add_message_param("Subst05", "67890", NULL, NULL, NULL);

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890", line, "The returned string is correct");

	/* Allowing for the terminator, there are 89 bytes left from the 100 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(89, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_full_length_param_msgs()
{
	char buffer[12];
	buffer[11] = '~';

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 11), "stringbuild_initialise() returns TRUE");
	stringbuild_add_message_param("Subst05", "67890", NULL, NULL, NULL);

	char *line = stringbuild_get_line();

	TEST_ASSERT_NOT_NULL_MESSAGE(line, "The returned line is not NULL");
	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234567890", line, "The returned string is correct");

	/* Allowing for the terminator, there are 0 bytes left from the 11 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_EQUAL_CHAR_MESSAGE('~', buffer[11], "The guard character hasn't changed");
	TEST_ASSERT_FALSE_MESSAGE(stringbuild_get_too_long(), "The too long flag is clear");
}

void test_initialise_write_and_read_over_length_param_msgs()
{
	char buffer[6];
	buffer[5] = '~';

	TEST_ASSERT_TRUE_MESSAGE(stringbuild_initialise(buffer, 5), "stringbuild_initialise() returns TRUE");
	stringbuild_add_message_param("Subst05", "67890", NULL, NULL, NULL);

	char *line = stringbuild_get_line();

	TEST_ASSERT_NULL_MESSAGE(line, "The returned line is NULL");
//	TEST_ASSERT_EQUAL_PTR_MESSAGE(buffer, line, "The returned pointer is correct");
//	TEST_ASSERT_EQUAL_STRING_MESSAGE("1234", line, "The returned string is correct");

	/* Allowing for the terminator, there are 0 bytes left from the 11 we supplied. */
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, stringbuild_get_remaining(), "The remaining space is correct");
	TEST_ASSERT_EQUAL_CHAR_MESSAGE('~', buffer[5], "The guard character hasn't changed");
	TEST_ASSERT_TRUE_MESSAGE(stringbuild_get_too_long(), "The too long flag is set");
}


// Write string + string into large buffer
// Write string + printf into large buffer
// Write string and read; reset; write string and read

/**
 * The main test runner.
 */

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_initialise_to_null_buffer);
	RUN_TEST(test_initialise_to_null_zero_lenth_buffer);
	RUN_TEST(test_initialise_to_zero_length_buffer);
	RUN_TEST(test_initialise_read_empty_line);
	RUN_TEST(test_initialise_write_and_read_empty_string);
	RUN_TEST(test_initialise_write_and_read_short_string);
	RUN_TEST(test_initialise_write_and_read_full_length_string);
	RUN_TEST(test_initialise_write_and_read_over_length_string);
	RUN_TEST(test_initialise_write_and_read_empty_printf);
	RUN_TEST(test_initialise_write_and_read_short_printf);
	RUN_TEST(test_initialise_write_and_read_full_length_printf);
	RUN_TEST(test_initialise_write_and_read_over_length_printf);
	RUN_TEST(test_initialise_write_and_read_empty_fixed_msgs);
	RUN_TEST(test_initialise_write_and_read_short_fixed_msgs);
	RUN_TEST(test_initialise_write_and_read_full_length_fixed_msgs);
	RUN_TEST(test_initialise_write_and_read_over_length_fixed_msgs);
	RUN_TEST(test_initialise_write_and_read_empty_param_msgs);
	RUN_TEST(test_initialise_write_and_read_short_param_msgs);
	RUN_TEST(test_initialise_write_and_read_full_length_param_msgs);
	RUN_TEST(test_initialise_write_and_read_over_length_param_msgs);
	return UNITY_END();
}
