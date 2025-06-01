compile:
	make -C deep_learning
	make -C xgboost
	(which fd > /dev/null && fd --no-ignore --hidden pdf) || (echo '==>' Try to find all pdfs manually)

clean:
	go run ~/Workspace/y/tools/delete_unused_dirs.go
