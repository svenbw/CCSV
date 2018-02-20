#ifndef CSV_INCLUDE
#define CSV_INCLUDE

#define CSV_SEEK_SET 0
#define CSV_SEEK_CUR 1

struct csv_data;

int csv_create(struct csv_data** csv);
int csv_destroy(struct csv_data** csv);
int csv_set_separator(struct csv_data *csv, const char separator);
int csv_set_end_of_line(struct csv_data *csv, const char* end_of_line);
int csv_add_column(struct csv_data *csv, const char* title, const char* key);
int csv_max_columns(struct csv_data *csv, const size_t count);
int csv_add_row(struct csv_data *csv);
int csv_add_cell(struct csv_data *csv, const char* value);
int csv_trim_rows(struct csv_data *csv);
int csv_set_current_cell_value(struct csv_data *csv, const char* value);
int csv_cell_seek(struct csv_data *csv, const size_t offset, int origin);
int csv_set_cell_value(struct csv_data *csv, const size_t index, const char* value);
int csv_set_cell_value_for_column(struct csv_data *csv, const char* key, const char* value);
int csv_flush(struct csv_data *csv);
int csv_flush_all(struct csv_data *csv);
int csv_fopen(struct csv_data *csv, const char *filename, const char *mode);
int csv_close(struct csv_data *csv);


#endif