clean:
	fd -I configure.mk -X rm
	go run ~/Workspace/y/tools/delete_unused_dirs.go --skip_dir c4
