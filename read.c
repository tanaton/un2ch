#include "read.h"

int main(){
	un2ch_t *init = un2ch_init();
	un2ch_get_server(init);
	un2ch_free(init);
	return 0;
}
