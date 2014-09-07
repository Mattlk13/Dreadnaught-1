// vfs.c -- Brad Slayter

#include "fs/vfs.h"

#define DEVICE_MAX 26

PFILESYSTEM _File_Systems[DEVICE_MAX];

FILE vol_open_file(const char *fname, FLAGS flags) {
	if (fname) {
		// default to device a
		unsigned char device = 'a';

		char *filename = (char *)fname;

		if (fname[1] == ':') { // if ':' the first char is device name
			device = fname[0];
			filename += 2; // strip device name
		}

		if (_File_Systems[device - 'a']) {
			// set vol specific info and return file
			FILE file = _File_Systems[device - 'a']->open(filename, flags);
			file.deviceID = device;
			return file;
		}
	}

	FILE file;
	file.flags = FS_INVALID;
	return file;
}

void vol_read_file(PFILE file, unsigned char *buffer, u32int length) {
	if (file) {
		if (_File_Systems[file->deviceID - 'a']) {
			_File_Systems[file->deviceID - 'a']->read(file, buffer, length);
		}
	}
}

void vol_write_file(PFILE file, unsigned char *buffer, u32int length) {
	if (file) {
		if (_File_Systems[file->deviceID - 'a']) {
			_File_Systems[file->deviceID - 'a']->write(file, buffer, length);
		}
	}
}

void vol_close_file(PFILE file) {
	if (file) {
		if (_File_Systems[file->deviceID - 'a']) {
			_File_Systems[file->deviceID - 'a']->close(file);
		}
	}
}

void vol_list_dir() {
	_File_Systems[0]->list();
}

void vol_register_file_system(PFILESYSTEM fsys, u32int deviceID) {
	static int i = 0;

	if (i < DEVICE_MAX) {
		if (fsys) {
			_File_Systems[deviceID] = fsys;
			i++;
		}
	}
}

void vol_unregister_file_system(PFILESYSTEM fsys) {
	for (int i = 0; i < DEVICE_MAX; i++) {
		if (_File_Systems[i] == fsys)
			_File_Systems[i] = 0;
	}
}

void vol_unregister_file_system_by_id(u32int deviceID) {
	if (deviceID < DEVICE_MAX)
		_File_Systems[deviceID] = 0;
}