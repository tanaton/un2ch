#include "read.h"

int main(){
	un2ch_t *init = un2ch_init();
	unstr_t *board_list = 0;
	unstr_t *line = 0;
	unstr_t *p1 = unstr_init_memory(32);
	unstr_t *p2 = unstr_init_memory(32);
	unstr_t *p3 = unstr_init_memory(32);
	unstr_t *data = 0;
	size_t index = 0;

	un2ch_get_server(init);
	board_list = unstr_file_get_contents(init->board_list);

	line = unstr_strtok(board_list, "\n", &index);
	while(line != NULL){
		if(unstr_sscanf(line, "$/$<>$", p1, p2, p3) == 3){
			un2ch_set_info(init, p1, p2, NULL);
			data = un2ch_get_data(init);
			if(data){
				printf("HTTP %ld OK %s/%s\n", init->code, p1->data, p2->data);
			} else {
				printf("error %s/%s\n", p1->data, p2->data);
			}
		}
		unstr_free(line);
		line = unstr_strtok(board_list, "\n", &index);
	}
	unstr_delete(6, board_list, line, p1, p2, p3, data);
	un2ch_free(init);
	return 0;
}
