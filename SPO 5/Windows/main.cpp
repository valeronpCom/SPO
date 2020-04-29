#include <windows.h>
#include <iostream>

using namespace std;

int main() {

	HMODULE dll;

	void (*concat)(void);

	dll = LoadLibrary("DLL.dll");
	if (!dll) {

		cout << "Failed to load DLL" << endl;
		return GetLastError();
	}

	concat = (void (*)(void))GetProcAddress(dll, "concat");
	if (!concat) {

		cout << "Failed to get DLL function" << endl;
		return GetLastError();
	}

	concat();

	if (!FreeLibrary(dll)) {

		cout << "Failed to free DLL" << endl;
		return GetLastError();
	}

	return 0;
}