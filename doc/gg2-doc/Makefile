export SP_ENCODING=XML

.PHONY: all clean alld htmld pdfd psd html-tarballd cleand allu htmlu pdfu psu html-tarballu cleanu

all: alld allu
alld: htmld pdfd psd html-tarballd
allu: htmlu pdfu psu html-tarballu

htmld:
	@rm -rf html/devel
	@mkdir -p html/devel
	( cd html/devel; docbook2html -p `which openjade` ../../sgmld/main.sgml )
	scripts/html-stylesheet.sh html/devel/
	cp docbook.css html/devel/
htmlu:
	@rm -rf html/user
	@mkdir -p html/user
	( cd html/user; docbook2html -p `which openjade` ../../sgmlu/main.sgml )
	scripts/html-stylesheet.sh html/user/
	cp docbook.css html/user/

html-tarballd: html/devel html/devel/index.html
	@mkdir -p tarballs
	tar -czf tarballs/gg2-develdoc.tar.gz html/devel/

html-tarballu: html/user html/user/index.html
	@mkdir -p tarballs
	tar -czf tarballs/gg2-userdoc.tar.gz html/user/

pdfd:
	@rm -rf pdf/gg2-develdoc*
	@mkdir -p pdf
	docbook2pdf -p `which openjade` sgmld/main.sgml
	mv main.pdf pdf/gg2-develdoc.pdf
	gzip --best -c pdf/gg2-develdoc.pdf > pdf/gg2-develdoc.pdf.gz

pdfu:
	@rm -rf pdf/gg2-userdoc*
	@mkdir -p pdf
	docbook2pdf -p `which openjade` sgmlu/main.sgml
	mv main.pdf pdf/gg2-userdoc.pdf
	gzip --best -c pdf/gg2-userdoc.pdf > pdf/gg2-userdoc.pdf.gz

psd:
	@rm -rf ps/gg2-develdoc*
	@mkdir -p ps
	docbook2ps -p `which openjade` sgmld/main.sgml
	mv main.ps ps/gg2-develdoc.ps
	gzip --best -c ps/gg2-develdoc.ps > ps/gg2-develdoc.ps.gz

psu:
	@rm -rf ps/gg2-userdoc*
	@mkdir -p ps
	docbook2ps -p `which openjade` sgmlu/main.sgml
	mv main.ps ps/gg2-userdoc.ps
	gzip --best -c ps/gg2-userdoc.ps > ps/gg2-userdoc.ps.gz

clean: cleand cleanu
	@rm -rf pdf
	@rm -rf ps
	@rm -rf tarballs

cleand:
	@rm -rf html/devel
	@rm -rf pdf/gg2-develdoc*
	@rm -rf ps/gg2-develdoc*
	@rm -rf tarballs/gg2-develdoc*

cleanu:
	@rm -rf html/user
	@rm -rf pdf/gg2-userdoc*
	@rm -rf ps/gg2-userdoc*
	@rm -rf tarballs/gg2-userdoc*
