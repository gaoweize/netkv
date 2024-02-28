import argparse


def split_file(input_file, output_prefix, lines_per_file=10000):
    with open(input_file, "r", encoding="utf-8") as file:
        lines = file.readlines()

    total_lines = len(lines)
    num_files = (total_lines + lines_per_file - 1) // lines_per_file

    for i in range(num_files):
        start_idx = i * lines_per_file
        end_idx = (i + 1) * lines_per_file
        output_file = f"{output_prefix}_{i+1}.txt"

        with open(output_file, "w", encoding="utf-8") as output:
            output.writelines(lines[start_idx:end_idx])


def main():
    parser = argparse.ArgumentParser(
        description="Split a text file into smaller files."
    )
    parser.add_argument("input_file", help="Path to the input text file")
    parser.add_argument("output_prefix", help="Prefix for the output files")
    parser.add_argument(
        "--lines_per_file",
        type=int,
        default=10000,
        help="Number of lines per output file (default: 10000)",
    )

    args = parser.parse_args()

    split_file(args.input_file, args.output_prefix, args.lines_per_file)


# python script.py input_file.txt output_prefix --lines_per_file 10000
if __name__ == "__main__":
    main()
