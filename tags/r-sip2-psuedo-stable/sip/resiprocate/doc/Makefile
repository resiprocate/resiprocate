#
# $Id: Makefile,v 1.5 2002/09/29 00:47:20 alan Exp $
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
$(OUTDIR)/%.ps: %.dot Makefile $(OUTDIR)
	$(DOT) -Tps -o$@ $<
$(OUTDIR)/%.png: %.dot
	$(DOT) -Tpng -o$@ $<
$(OUTDIR)/%.svg: %.dot Makefile $(OUTDIR)
	$(DOT) -Tsvg -o$@ $<

# Graphs
$(OUTDIR)/%.ps: %.neato Makefile $(OUTDIR)
	$(NEATO) -Tps -o$@ $<
$(OUTDIR)/%.png: %.neato
	$(NEATO) -Tpng -o$@ $<
$(OUTDIR)/%.svg: %.neato Makefile $(OUTDIR)
	$(NEATO) -Tsvg -o$@ $<

# Code autogen
%.dot: ../%.cxx fsm-dot.awk
	awk -f fsm-dot.awk $< > $@

clean: 
	-/bin/rm -f $(FILES) *.ps

dummy:
	cp  *.pdf  ~/public_html/misc/


test:
	-@echo $(FILES)
