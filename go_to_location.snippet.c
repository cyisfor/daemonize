#ifndef RESOURCE_CREATE
#define RESOURCE_CREATE 0
#else
#define RESOURCE_DERP RESOURCE_PERM | O_CREAT
#endif
	if(info.locations.RESOURCE.len) {
		const char* loc = ZSTR(info.locations.RESOURCE);
		mkdir(loc, 0700);
		ensure0(chdir(loc));
	} else if(uid == 0) {
		ensure0(chdir(DEFAULT_ROOT_LOC));
	} else {
		home =	getenv("HOME");
		if(home == NULL) {
			struct passwd* u = getpwuid(uid);
			ensure_ne(NULL, u);
			home = u->pw_dir;
			/* ensure_ne(NULL, home); */
		}
		ensure0(chdir(home));
		ensure0(chdir(DEFAULT_USER_LOC));
	}

	stradd(&filename, "." STRIFY(RESOURCE) "\0");
	fds.RESOURCE = open((const char*)filename.base,
						RESOURCE_DERP, RESOURCE_CREATE);
	ensure_ge(fds.RESOURCE, 0);
	filename.len = savepoint;

#undef RESOURCE
#undef RESOURCE_PERM
#undef RESOURCE_CREATE
#ifdef RESOURCE_DERP
#undef RESOURCE_DERP
#endif
#undef DEFAULT_ROOT_LOC
#undef DEFAULT_USER_LOC
