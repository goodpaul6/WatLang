@echo off
if not defined DevEnvDir (
    call "%VS_CMD_LINE_BUILD_PATH%" x64
)

SET opts=-Zi /W3 /EHsc /Fewat.exe /D_CRT_SECURE_NO_WARNINGS

pushd bin
    cl.exe ..\main.cc %opts%
popd
