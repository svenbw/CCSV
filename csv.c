#include <sglib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "csv.h"


struct csv_column {
  char* title;
  char* key;

  struct csv_column* next;
};

enum export_flags {
  column_exported = 1,
  column_has_title = 2,
  file_flushed = 4
};

struct csv_cell {
  char* value;
};

struct csv_row {
  struct csv_cell* cells;
  size_t cell_cursor;

  struct csv_row* next;
};

struct csv_data {
  struct csv_column* columns;
  size_t column_count;
  struct csv_row* rows;
  struct csv_row* current_row;
  char separator;
  char end_of_line[4];
  FILE *file;
  enum export_flags export_flags;
};



static int _csv_set_cell_value(struct csv_cell* cell, const char* value)
{
  if (value == NULL)
    return 0;

  cell->value = (char*) malloc(strlen(value)+1);
  if (cell->value == NULL)
    return -3;

  strcpy(cell->value, value);

  return 0;
}

static void _csv_free_cell(struct csv_cell* cell)
{
  if (cell->value != NULL)
  {
    free(cell->value);
    cell->value = NULL;
  }
}


/**
  Create a new csv object

  @param csv Csv object
  @return 0 on success
*/
int csv_create(struct csv_data** csv)
{
  *csv = (struct csv_data*) malloc(sizeof(struct csv_data));
  if (*csv == NULL)
    return -1;

  memset(*csv, 0, sizeof(struct csv_data));

  (*csv)->separator = ',';
  strcpy((*csv)->end_of_line, "\n");
  (*csv)->file = stdout;

  return 0;
}

/**
  Clean up the csv object

  @param csv Csv object
  @return 0 on success
*/
int csv_destroy(struct csv_data** csv)
{
  size_t index;
  struct csv_cell *cell;

  if (*csv == NULL)
    return 0;
  
  SGLIB_LIST_MAP_ON_ELEMENTS(struct csv_column, (*csv)->columns, column, next, {
    SGLIB_LIST_DELETE(struct csv_column, (*csv)->columns, column, next);

    free(column->title);
    if (column->key != NULL)
    {
      free(column->key);
    }
    
    free(column);
    column = NULL;
  });

  SGLIB_LIST_MAP_ON_ELEMENTS(struct csv_row, (*csv)->rows, row, next, {
    SGLIB_LIST_DELETE(struct csv_row, (*csv)->rows, row, next);

    for (index=0, cell=row->cells; index<(*csv)->column_count; ++index, ++cell)
    {
      _csv_free_cell(cell);
    }
    free(row->cells);
    
    free(row);
    row = NULL;
  });

  if (((*csv)->file != NULL) && ((*csv)->file != stdout))
  {
    fclose((*csv)->file);
  }
  
  free(*csv);

  *csv = NULL;
  return 0;
}

/**
  Set the cell separator of the csv

  @param csv Csv object
  @param separator Set the separator character
  @return 0 on success
*/
int csv_set_separator(struct csv_data *csv, const char separator)
{
  if ((csv->export_flags & file_flushed) != 0)
    return -1;

  csv->separator = separator; 
  
  return 0;
}

/**
  Set the end of line character(s) for the csv

  @param csv Csv object
  @param end_of_line End of line character(s) to use
  @return 0 on success
*/
int csv_set_end_of_line(struct csv_data *csv, const char* end_of_line)
{
  if ((csv->export_flags & file_flushed) != 0)
    return -1;

  if (strlen(end_of_line) >= 4)
    return -1;

  strcpy(csv->end_of_line, end_of_line);
  
  return 0;
}

/**
  Add a column to the csv

  @param csv Csv object
  @param title Title of the column
  @param key Key name of the column
  @return 0 on success
*/
int csv_add_column(struct csv_data *csv, const char* title, const char* key)
{
  struct csv_column *column;

  if ((csv->export_flags & file_flushed) != 0)
    return -2;

  column = (struct csv_column*) malloc(sizeof(struct csv_column));
  if (column == NULL)
    return -1;

  memset(column, 0, sizeof(struct csv_column));

  if (title != NULL)
  {
    column->title = (char*) malloc(strlen(title) + 1);
    strcpy(column->title, title);

    csv->export_flags |= column_has_title;
  }

  if (key != NULL)
  {
    column->key = (char*) malloc(strlen(key) + 1);
    strcpy(column->key, key);
  }

  SGLIB_LIST_CONCAT(struct csv_column, csv->columns, column, next);

  // Update the column count
  SGLIB_LIST_LEN(struct csv_column, csv->columns, next, csv->column_count);

  // If there are rows, increase the cell size
  if (csv->rows != NULL)
  {
    SGLIB_LIST_MAP_ON_ELEMENTS(struct csv_row, csv->rows, row, next, {
      row->cells = realloc(row->cells, csv->column_count * sizeof(struct csv_cell));
      
      memset(&row->cells[csv->column_count-1], 0, sizeof(struct csv_cell));
    });
  }

  return 0;
}

/**
  Set the maximum number of columns

  @param csv Csv object
  @param count Number of columns
  @return 0 on success
*/
int csv_max_columns(struct csv_data *csv, const size_t count)
{
  size_t missing;
  struct csv_column *column;
  
  if (count < csv->column_count)
    return -1;

  if (csv->rows != NULL)
    return -2;
  
  if ((csv->export_flags & file_flushed) != 0)
    return -3;

  for (missing = count-csv->column_count; missing > 0; --missing)
  {
    column = (struct csv_column*) malloc(sizeof(struct csv_column));
    if (column == NULL)
      return -4;

    memset(column, 0, sizeof(struct csv_column));
    
    SGLIB_LIST_CONCAT(struct csv_column, csv->columns, column, next);
  }

  // Update the column count
  SGLIB_LIST_LEN(struct csv_column, csv->columns, next, csv->column_count);
  
  return 0;
}

/**
  Add a new row to the csv

  @param csv Csv object
  @return 0 on success
*/
int csv_add_row(struct csv_data *csv)
{
  struct csv_row *row = (struct csv_row*) malloc(sizeof(struct csv_row));
  if (row == NULL)
    return -1;

  memset(row, 0, sizeof(struct csv_row));

  row->cells = (struct csv_cell*) malloc(csv->column_count * sizeof(struct csv_cell));
  if (row->cells == NULL)
  {
    free(row);
    return -2;
  }

  memset(row->cells, 0, csv->column_count * sizeof(struct csv_cell));
  row->cell_cursor = 0;

  SGLIB_LIST_CONCAT(struct csv_row, csv->rows, row, next);

  csv->current_row = row;

  return 0;
}

/**
  Trim unused columns

  @param csv Csv object
  @return Number of columns
*/
int csv_trim_rows(struct csv_data *csv)
{
  size_t max_cell_count = 0;
  size_t index;

  index = 0;
  SGLIB_LIST_MAP_ON_ELEMENTS(struct csv_column, csv->columns, column, next, {
    ++index;

    if (column->title != NULL)
    {
      if (index > max_cell_count)
        max_cell_count = index;
    }
  });

  SGLIB_LIST_MAP_ON_ELEMENTS(struct csv_row, csv->rows, row, next, {
    for (index=csv->column_count; index>=0; --index)
    {
      if (row->cells[index].value != NULL)
      {
        if (index > max_cell_count)
          max_cell_count = index;

        break;
      }
    }
  });
  
  csv->column_count = max_cell_count;

  return max_cell_count;
}

/**
  Get the current cell

  @param csv Csv object
  @return 0 on success
*/
static struct csv_cell* csv_get_current_cell(struct csv_data *csv)
{
  if (csv->current_row == NULL)
    return NULL;

  if (csv->current_row->cell_cursor >= csv->column_count)
    return NULL;
  
  return &csv->current_row->cells[csv->current_row->cell_cursor];
}

/**
  Gets a cell based on the column index

  @param csv Csv object
  @param index Column index
  @return 0 on success
*/
static struct csv_cell* csv_get_cell_at(struct csv_data *csv, size_t index)
{
  if (csv->current_row == NULL)
    return NULL;

  if (index >= csv->column_count)
    return NULL;
  
  return &csv->current_row->cells[index];
}

/**
  Gets a cell based on the column key

  @param csv Csv object
  @param key Value of the column key to check
  @return 0 on success
*/
static struct csv_cell* csv_get_cell_for_column(struct csv_data *csv, const char* key)
{
  struct csv_column *column = csv->columns;
  struct csv_cell *cell;

  if (csv->current_row == NULL)
    return NULL;

  cell = csv->current_row->cells;
  while (column != NULL)
  {
    if ((column->key != NULL) && (strcmp(column->key, key) == 0))
      break;

    column = column->next;
    ++cell;
  }

  if (column == NULL)
    return NULL;

  return cell;
}

/**
  Add a new cell on the stack

  @param csv Csv object
  @param value Value of the cell
  @return 0 on success
*/
int csv_add_cell(struct csv_data *csv, const char* value)
{
  struct csv_cell* cell;
  int result;
  size_t position;

  if (csv->current_row == NULL)
  {
    if (csv_add_row(csv) < 0)
      return -1;
  }
  
  cell = csv_get_current_cell(csv);
  if (cell == NULL)
    return -2;

  if ((result = _csv_set_cell_value(cell, value)) < 0)
    return result;

  position = csv->current_row->cell_cursor;
  ++csv->current_row->cell_cursor;

  return position;
}

/**
  Set the value of the current cell

  @param csv Csv object
  @param value Value of the cell
  @return 0 on success
*/
int csv_set_current_cell_value(struct csv_data *csv, const char* value)
{
  struct csv_cell* cell;

  if ((cell = csv_get_cell_at(csv, csv->current_row->cell_cursor)) == NULL)
    return -1;
  
  _csv_free_cell(cell);

  return _csv_set_cell_value(cell, value);
}

/**
  Set the current cell pointer

  @param csv Csv object
  @param offset Column offset
  @param origin Position to start
  @return 0 on success
*/
int csv_cell_seek(struct csv_data *csv, const size_t offset, int origin)
{
  if (csv->current_row == NULL)
    return -1;

  if (origin == CSV_SEEK_SET)
  {
    if (offset >= csv->column_count)
      return -2;

    csv->current_row->cell_cursor = offset;
  }
  else if (origin == CSV_SEEK_CUR)
  {
    if (csv->current_row->cell_cursor + offset >= csv->column_count)
      return -2;

    csv->current_row->cell_cursor += offset;
  }

  return csv->current_row->cell_cursor;
}

/**
  Set the value of a cell using the column index

  @param csv Csv object
  @param index Column index
  @param value Value of the cell
  @return 0 on success
*/
int csv_set_cell_value(struct csv_data *csv, const size_t index, const char* value)
{
  struct csv_cell* cell;

  if ((cell = csv_get_cell_at(csv, index)) == NULL)
    return -1;
  
  _csv_free_cell(cell);

  return _csv_set_cell_value(cell, value);
}

/**
  Set the value of a cell using the column key value

  @param csv Csv object
  @param key Value of the column key to check
  @param value Value of the cell
  @return 0 on success
*/
int csv_set_cell_value_for_column(struct csv_data *csv, const char* key, const char* value)
{
  struct csv_cell* cell;

  cell = csv_get_cell_for_column(csv, key);
  if (cell == NULL)
    return -1;
  
  _csv_free_cell(cell);

  return _csv_set_cell_value(cell, value);
}

static void _csv_write_cell(struct csv_data *csv, const char* value)
{
  if ((strchr(value, '\n') != 0) ||
      (strchr(value, '"') != 0) ||
      (strchr(value, csv->separator) != 0))
  {
    fprintf(csv->file, "\"%s\"", value);
  }
  else
  {
    fprintf(csv->file, "%s", value);
  }
}

static int _csv_flush(struct csv_data *csv, const int all)
{
  struct csv_column* column = csv->columns;
  struct csv_cell* cell;
  size_t index;

  if (column == NULL)
    return 0;

  // Export columns
  if (((csv->export_flags & column_exported) == 0) &&
      ((csv->export_flags & column_has_title) != 0))
  {
    if (column->title != NULL)
    {
      fprintf(csv->file, "%s", column->title);
    }

    column = column->next;
    while (column != NULL)
    {
      fprintf(csv->file, "%c", csv->separator);
      if (column->title != NULL)
      {
        _csv_write_cell(csv, column->title);
      }

      column = column->next;
    };
    fprintf(csv->file, "%s", csv->end_of_line);
    csv->export_flags |= column_exported | file_flushed;
  }

  // Export rows
  SGLIB_LIST_MAP_ON_ELEMENTS(struct csv_row, csv->rows, row, next, {
    
    // Flush the last row if final cleanup
    if ((csv->current_row == row) && (! all))
      break;

    if (row->cells[0].value != NULL)
    {
      fprintf(csv->file, "%s", row->cells[0].value);
    }

    for (index=1; index<csv->column_count; ++index)
    {
      fprintf(csv->file, "%c", csv->separator);
      if (row->cells[index].value != NULL)
      {
        _csv_write_cell(csv, row->cells[index].value);
      }
    }
    fprintf(csv->file, "%s", csv->end_of_line);
    
    csv->export_flags |= file_flushed;
  });

  // Remove flushed data
  SGLIB_LIST_MAP_ON_ELEMENTS(struct csv_row, csv->rows, row, next, {

    // Flush the last row if final cleanup
    if ((csv->current_row == row) && (! all))
      break;
    
    SGLIB_LIST_DELETE(struct csv_row, csv->rows, row, next);

    for (index=0, cell=row->cells; index<csv->column_count; ++index, ++cell)
    {
      _csv_free_cell(cell);
    }
    free(row->cells);
    
    free(row);
    row = NULL;
  });

  if (all)
  {
    csv->current_row = NULL;
  }

  return 0;
}

/**
  Write closed rows and column titles to the output

  @param csv Csv object
  @return 0 on success
*/
int csv_flush(struct csv_data *csv)
{
  return _csv_flush(csv, 0);
}

/**
  Write ALL rows to the output

  @param csv Csv object
  @return 0 on success
*/
int csv_flush_all(struct csv_data *csv)
{
  return _csv_flush(csv, 1);
}

/**
  Write the csv output to a file

  @param csv Csv object
  @param filename Filename to write to
  @param mode File open mode
  @return 0 on success
*/
int csv_fopen(struct csv_data *csv, const char *filename, const char *mode)
{
  csv->file = fopen(filename, mode);
  if (csv->file == NULL)
    return -1;

  return 0;
}

/**
  Close the current file

  @param csv Csv object
  @return 0 on success
*/
int csv_close(struct csv_data *csv)
{
  if (csv->file == NULL)
    return 0;

  _csv_flush(csv, 1);

  fclose(csv->file);
  
  csv->file = NULL;

  return 0;
}
