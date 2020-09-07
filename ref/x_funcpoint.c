#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

void test_function(bool (*function_pointer) (int x)) {
	printf("addr passed function_pointer %p\n", function_pointer);
	if (function_pointer(100)) {
		printf("  run: true\n");
	} else {
		printf("  run: false\n");
	}
}

bool function_outside_main(int x)
{
	return x < 0;
}

int main(void)
{
	// run with function defined globally
	printf("addr function_outside_main %p\n", function_outside_main);
	test_function(function_outside_main);

	// run with function defined in this stack block
	bool function_inside_main(int x) {
		return x > 0;
	}
	printf("addr function_inside_main %p\n", function_inside_main);
	test_function(function_inside_main); // shouldn't the address be valid?
}
