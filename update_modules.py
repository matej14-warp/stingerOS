import os


MODULES_DIR = "modules"
GRUB_CFG = "boot/grub.cfg"
START_MARKER = "
END_MARKER = "

def update_grub():
    if not os.path.exists(MODULES_DIR):
        print(f"Error: {MODULES_DIR} directory not found.")
        return


    module_lines = []
    for root, dirs, files in os.walk(MODULES_DIR):
        for file in files:

            full_path = os.path.join(root, file)

            rel_path = os.path.relpath(full_path, MODULES_DIR).replace("\\", "/")

            iso_path = "/" + rel_path

            line = f'    module {iso_path} "{iso_path}"'
            module_lines.append(line)

    if not module_lines:
        print("No modules found in modules/ directory.")
    else:
        print(f"Found {len(module_lines)} modules.")


    if not os.path.exists(GRUB_CFG):
        print(f"Error: {GRUB_CFG} not found.")
        return

    with open(GRUB_CFG, "r") as f:
        lines = f.readlines()


    new_lines = []
    in_block = False
    found_markers = False

    for line in lines:
        stripped = line.strip()

        if stripped == START_MARKER.strip():
            new_lines.append(line)

            for m in module_lines:
                new_lines.append(m + "\n")
            in_block = True
            found_markers = True
            continue

        if stripped == END_MARKER.strip():
            new_lines.append(line)
            in_block = False
            continue

        if not in_block:
            new_lines.append(line)

    if not found_markers:
        print(f"Error: Could not find markers in {GRUB_CFG}")
        return


    with open(GRUB_CFG, "w") as f:
        f.writelines(new_lines)

    print("Successfully updated grub.cfg with current modules.")

if __name__ == "__main__":
    update_grub()




