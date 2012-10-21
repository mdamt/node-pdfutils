#define BUILDING_NODE_EXTENSION
#include <node.h>
#include <node_buffer.h>
#include <poppler.h>
#include <stdlib.h>
#include <cairo.h>
#include <cairo-svg.h>
#include "page.h"
#include "page_job.h"
#include "document.h"

using namespace v8;
using namespace node;

Persistent<Function> PageJob::constructor;

PageJob::PageJob(Page &page, Format format) {
	this->page = &page;
	this->format = format;

	const unsigned argc = 1;
	Handle<Value> argv[argc] = {
		this->page->handle_
	};
	Persistent<Object> instance = Persistent<Object>::New((*constructor)->NewInstance(argc, argv));
	this->Wrap(instance);
}

PageJob::~PageJob() {
}

void PageJob::Init(Handle<Object> target) {
	Local<FunctionTemplate> tpl = FunctionTemplate::New();
	tpl->SetClassName(String::NewSymbol("PDFJob"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("PageJob"), constructor);
}


void PageJob::run() {
	cairo_surface_t *surface;
	cairo_t *cr;

	if (this->page->pg == NULL) {
		// TODO ERROR
		return; 
	}   

	if(this->format == FORMAT_SVG)
		surface = cairo_svg_surface_create_for_stream(PageJob::ProcChunk, this, this->page->w, this->page->h);
	else
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, this->page->w, this->page->h);

	cr = cairo_create(surface);
	poppler_page_render(this->page->pg, cr);
	if(this->format == FORMAT_PNG)
		cairo_surface_write_to_png_stream(surface, PageJob::ProcChunk, this);
	cairo_show_page(cr);
	cairo_destroy(cr);

	if(cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		// TODO ERROR
	}
	cairo_surface_destroy(surface);
}

cairo_status_t PageJob::ProcChunk(void *closure, const unsigned char *data, unsigned int length) {
	PageJob *self = (PageJob *)closure;

	self->page->document->addChunk(self, data, length);

	return CAIRO_STATUS_SUCCESS;
}

void PageJob::done() {
	this->MakeWeak();
}
