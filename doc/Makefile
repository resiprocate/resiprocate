#
# $Id: Makefile,v 1.4 2002/09/23 00:09:24 fluffy Exp $
#
NEATO=neato
DOT=dot

FSMS=srv-inv-fsm srv-inv-tree Preparse 
OUTDIR=out

PSFILES=$(patsubst %, $(OUTDIR)/%.ps, $(FSMS))
PDFFILES=$(patsubst %, $(OUTDIR)/%.pdf, $(FSMS))
SVGFILES=$(patsubst %, $(OUTDIR)/%.svg, $(FSMS))
PNGFILES=$(patsubst %, $(OUTDIR)/%.png, $(FSMS))

FILES=$(PSFILES) $(PDFFILES) $(SVGFILES) $(PNGFILES)



.phony:

all: $(FILES)

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
