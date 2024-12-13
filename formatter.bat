@echo off
echo Running clang-format on all C++ files...

for /r source %%f in (*.cpp *.hpp *.h) do (
    echo Formatting %%f
    clang-format -style=file -i "%%f"
)

for /r test %%f in (*.cpp *.hpp *.h) do (
    echo Formatting %%f
    clang-format -style=file -i "%%f"
)

echo Format complete!
pause