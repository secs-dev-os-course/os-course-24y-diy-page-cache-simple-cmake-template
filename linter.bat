@echo off
echo Checking code style...
for /r source %%f in (*.cpp *.hpp) do (
    echo Checking %%f
    clang-tidy -p build/compile_commands.json "%%f"
)
pause