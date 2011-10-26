
EXPORTS = SDL_image.x
HEADERS = \
	../../SDL_image.h

all: $(EXPORTS)

$(EXPORTS): $(HEADERS)
	perl gendef.pl $(HEADERS) >$@ || rm $@

clean:
	rm -f $(EXPORTS)
