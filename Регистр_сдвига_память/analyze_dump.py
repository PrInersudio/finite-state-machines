from json import loads
from sys import argv, stderr

if len(argv) < 2:
    print("Запускать:", argv[0], "<json файл дампа>", file=stderr)
    exit(1)
try:
    with open(argv[1], encoding="utf-8") as f:
        dump = loads(f.read())
    with open(argv[1].split(".")[0] + ".log", "w", encoding="utf-8") as f:
        for element in sorted(dump, key = lambda element: tuple(map(int, element["key"].split('_')))[::-1]):
            print(element["key"], element["members"], file = f)
except Exception as e:
    print(f"Ошибка обработки файла: {e}", file=stderr)
    exit(1)
