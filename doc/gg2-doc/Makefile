export SP_ENCODING=XML

.PHONY: all clean alld htmld pdfd psd html-tarballd cleand allu htmlu pdfu psu html-tarballu cleanu

all: alld allu
alld: htmld pdfd psd html-tarballd
allu: htmlu pdfu psu html-tarballu

htmld:
	@rm -rf html/devel
	@mkdir -p html/devel
	docbook2html -p `which openjade` -d dtd.dsl#html sgmld/main.sgml -o html/devel -c /etc/sgml/catalog
	scripts/html-stylesheet.sh html/devel/
	cp docbook.css html/devel/
htmlu:
	@rm -rf html/user
	@mkdir -p html/user
	docbook2html -p `which openjade` -d dtd.dsl#html sgmlu/main.sgml -o html/user -c /etc/sgml/catalog
	scripts/html-stylesheet.sh html/user/
	cp docbook.css html/user/
	rm -rf html/user/img
	test -f sgmlu/images && mkdir html/user/img && cp `cat sgmlu/images | sed "s/\(.*\)/sgmlu\/\1/g"` html/user/img || echo ""

html-tarballd: html/devel/index.html
	@mkdir -p tarballs
	tar -czf tarballs/gg2-develdoc.tar.gz html/devel/

html-tarballu: html/user/index.html
	@mkdir -p tarballs
	tar -czf tarballs/gg2-userdoc.tar.gz html/user/

pdfd:
	@rm -rf pdf/gg2-develdoc*
	@mkdir -p pdf
	docbook2pdf -p `which openjade` -d dtd.dsl#print sgmld/main.sgml -o pdf -c /etc/sgml/catalog
	mv pdf/main.pdf pdf/gg2-develdoc.pdf
	gzip --best -c pdf/gg2-develdoc.pdf > pdf/gg2-develdoc.pdf.gz

pdfu:
	@rm -rf pdf/gg2-userdoc*
	@mkdir -p pdf
	docbook2pdf -p `which openjade` -d dtd.dsl#print sgmlu/main.sgml -o pdf -c /etc/sgml/catalog
	mv pdf/main.pdf pdf/gg2-userdoc.pdf
	gzip --best -c pdf/gg2-userdoc.pdf > pdf/gg2-userdoc.pdf.gz

psd:
	@rm -rf ps/gg2-develdoc*
	@mkdir -p ps
	docbook2ps -p `which openjade` -d dtd.dsl#print sgmld/main.sgml -o ps -c /etc/sgml/catalog
	mv ps/main.ps ps/gg2-develdoc.ps
	gzip --best -c ps/gg2-develdoc.ps > ps/gg2-develdoc.ps.gz

psu:
	@rm -rf ps/gg2-userdoc*
	@mkdir -p ps
	docbook2ps -p `which openjade` -d dtd.dsl#print sgmlu/main.sgml -o ps -c /etc/sgml/catalog
	mv ps/main.ps ps/gg2-userdoc.ps
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
