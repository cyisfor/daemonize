// bleh
char* concat(int num, ...) {
	va_list args;
	va_start(args,num);
	const char** elements = (const char**) malloc(num*sizeof(const char*));
	if(!elements) return NULL;
	ssize_t length;
	int i;
	for(i=0;i<num;++i) {
		elements[i] = va_arg(args,const char*);
		length += strlen(elements[i]);
	}
	char* dest = (char*) malloc(length);
	if(!dest) return NULL;

	char* cur = dest;
	for(i=0;i<num;++i) {
		cur = stpcpy(dest,elements[i]);
	}
	free(elements);
	return dest;
}


