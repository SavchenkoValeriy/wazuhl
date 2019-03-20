with open("o2actions") as f:
    lines = [l.split("#")[0].strip() for l in f.readlines()]
    print(len(lines))
    print(", ".join(['"{}"'.format(l) for l in lines]))