#
# $Id: Makefile,v 1.7 2002/12/23 18:04:07 alan Exp $
#
NEATO=neato
DOT=dot

FSMS=srv-inv-fsm srv-inv-tree Preparse 
OUTDIR=out

PSFILES=$(patsubst %, $(OUTDIR)/%.ps, $(FSMS))
PDFFILES=$(patsubst %, $(OUTDIR)/%.pdf, $(FSMS))
SVGFILES=$(patsubst %, $(OUTDIR)/%.svg, $(FSMS))
PNGFILES=$(patsubst %, $(OUTDIR)/%.png, $(FSMS))

DOCBOOKTARGETS=design-overview.html
XSLP=xsltproc
XSLPOPT=--xinclude 
HTMLSHEET=htmlcss.xsl

FILES=$(PSFILES) $(PDFFILES) $(SVGFILES) $(PNGFILES) $(DOCBOOKTARGETS)


.PRECIOUS: Preparse.svg.dot Preparse.ps.dot Preparse.png.dot

.phony:

all: $(FILES)

%.html: $(HTMLSHEET) %.xml
	$(XSLP) $(XSLPOPT) -o $@ $^

#design-overview.html: $(HTMLSHEET) design-overview.xml
#	$(XSLP) $(XSLPOPT) -o $@ $^

$(OUTDIR):
	-mkdir $@

$(OUTDIR)/%.pdf: $(OUTDIR)/%.ps Makefile $(OUTDIR)
	ps2pdf13 $< $@

#Digraphs
$(OUTDIR)/%.ps: %.ps.dot Makefile $(OUTDIR)
	$(DOT) -Tps -o$@ $<

$(OUTDIR)/%.png: %.png.dot
	$(DOT) -Tpng -o$@ $<

$(OUTDIR)/%.svg: %.svg.dot Makefile $(OUTDIR)
	$(DOT) -Tsvg -o$@ $<

# Graphs
$(OUTDIR)/%.ps: %.neato Makefile $(OUTDIR)
	$(NEATO) -Tps -o$@ $<
$(OUTDIR)/%.png: %.neato
	$(NEATO) -Tpng -o$@ $<
$(OUTDIR)/%.svg: %.neato Makefile $(OUTDIR)
	$(NEATO) -Tsvg -o$@ $<

# Old style diagrams
%.ps.dot: %.dot
	ln -s $(@:.ps.dot=.dot) $@

%.png.dot: %.dot
	ln -s $(@:.png.dot=.dot) $@

%.svg.dot: %.dot
	ln -s $(@:.svg.dot=.dot) $@

# Code autogen
%.ps.dot: ../%.cxx fsm-dot.awk Makefile
	awk --assign output=$(@:.dot=) -f fsm-dot.awk $< > $@

%.svg.dot: ../%.cxx fsm-dot.awk Makefile
	awk --assign output=$(@:.dot=) -f fsm-dot.awk $< > $@


%.png.dot: ../%.cxx fsm-dot.awk Makefile
	awk --assign output=$(@:.dot=) -f fsm-dot.awk $< > $@


clean: 
	-/bin/rm -f $(FILES) *.ps

dummy:
	cp  *.pdf  ~/public_html/misc/


test:
	-@echo $(FILES)
