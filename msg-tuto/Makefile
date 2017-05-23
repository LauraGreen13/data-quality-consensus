index.html: README.org
	emacs -batch $< --funcall org-export-as-html
	mv README.html index.html
	sed -i 's/Times, serif/sans/' $@

msg-tuto.tgz: msg-tuto-solution.tgz
	cp $< foo.tgz
	gunzip foo.tgz
	tar --delete --file=foo.tar msg-tuto/src/masterworker1.c msg-tuto/src/masterworker2.c msg-tuto/src/masterworker3.c msg-tuto/src/masterworker4.c
	gzip foo.tar
	mv foo.tar.gz $@


git archive --prefix=msg-tuto/ --format tgz --output $@ master

msg-tuto-solution.tgz: 
	git archive --prefix=msg-tuto/ --format tgz --output $@ master

publish: msg-tuto.tgz msg-tuto-solution.tgz index.html img
	rsync -a --rsh=ssh $^ intra-id:/www-id/sites/mescal/www/membres/arnaud.legrand/msg-tuto/


