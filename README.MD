# CCSV: Create CSV files in C

CCSV is a simple source and header file to create CSV files from C.

## Features
- Only 2 files.
- Support for column titles.
- Add cell by index.
- Add cell by column key.
- Caches all rows in memory until a flush is called.


## Sample usage
- Create the csv class by calling `csv_create`
- Add columns `csv_add_column`
- Add cells using `csv_push_cell` or `csv_set_cell_value`
- Flush rows `csv_flush` (frees up memory)
- Destroy csv `csv_destroy`
