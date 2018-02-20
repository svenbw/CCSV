#include <sglib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../csv.h"


int main(int argc, char **argv)
{
  struct csv_data* csv;

  csv_create(&csv);

  /*
  // Uncomment to export to a csv file, else stdout is used
  if (csv_fopen(csv, "test.csv", "w") < 0)
  {
    fprintf(stderr, "FAIL");
    exit(1);
  }
  */

  csv_set_separator(csv, ';');
  csv_set_end_of_line(csv, "\n");

  csv_add_column(csv, "Col 1", "a");
  csv_add_column(csv, "Col 2", "b");
  csv_add_column(csv, "Col 3", "c");
  csv_add_column(csv, "Col 4", "d");
  csv_add_column(csv, "Col 5", "e");
  csv_add_column(csv, NULL, "f");
  csv_add_column(csv, "Col 7", "g");
  csv_add_column(csv, "Col 8", "h");
  
  // Set the maximum number of columns
  // csv_max_columns(csv, 20);
  // csv_trim_rows(csv);

  csv_add_cell(csv, "cell1");
  csv_add_cell(csv, "cell2");
  
  csv_set_current_cell_value(csv, "cell3");
  csv_set_cell_value(csv, 4, "cell5");

  // Second row
  csv_add_row(csv);
  csv_set_cell_value(csv, 4, "row2-5");

  csv_set_cell_value_for_column(csv, "h", "row2,8");

  // Fail
  csv_set_cell_value(csv, 14, "row2-5");

  csv_add_column(csv, "Col 9", "i");
  csv_set_cell_value_for_column(csv, "i", "row2-9");

  csv_flush(csv);

  csv_add_row(csv);
  csv_set_cell_value_for_column(csv, "h", "row1-8");

  csv_flush(csv);
  csv_close(csv);

  csv_destroy(&csv);

  return 0;
}