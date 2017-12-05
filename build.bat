cl /c copyHook.cpp /D _UNICODE
Link /dll copyHook.obj
cl jis2u16.cpp /D _UNICODE
