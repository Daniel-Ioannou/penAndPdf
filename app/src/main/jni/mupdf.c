#include <jni.h>
#include <time.h>
#include <pthread.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#ifdef NDK_PROFILER
#include "prof.h"
#endif

#include "mupdf/fitz.h"
#include "mupdf/pdf.h"

#define JNI_FN(A) Java_com_cgogolin_penandpdf_ ## A
#define PACKAGENAME "com/cgogolin/penandpdf"

#define LOG_TAG "libmupdf"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGT(...) __android_log_print(ANDROID_LOG_INFO,"alert",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

/* Enable to log rendering times (render each frame 100 times and time) */
#undef TIME_DISPLAY_LIST

#define MAX_SEARCH_HITS (500)
#define NUM_CACHE (3)
#define STRIKE_HEIGHT (0.375f)
#define UNDERLINE_HEIGHT (0.075f)
#define LINE_THICKNESS (0.07f)
#define INK_THICKNESS (3.0f)
#define INK_COLORr (1.0f)
#define INK_COLORg (0.0f)
#define INK_COLORb (0.0f)
#define HIGHLIGHT_COLORr (1.0f)
#define HIGHLIGHT_COLORg (1.0f)
#define HIGHLIGHT_COLORb (0.0f)
#define UNDERLINE_COLORr (0.0f)
#define UNDERLINE_COLORg (0.0f)
#define UNDERLINE_COLORb (1.0f)
#define STRIKEOUT_COLORr (1.0f)
#define STRIKEOUT_COLORg (0.0f)
#define STRIKEOUT_COLORb (0.0f)
#define TEXTANNOTICON_COLORr (0.0f)
#define TEXTANNOTICON_COLORg (0.0f)
#define TEXTANNOTICON_COLORb (1.0f)

#define SMALL_FLOAT (0.00001)
#define PROOF_RESOLUTION (300)



enum
{
    NONE,
    TEXT,
    LISTBOX,
    COMBOBOX,
    SIGNATURE
};

static const jchar PDFDocEncoding[] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,  
    0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,  
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,  
    0x02d8, 0x02c7, 0x02c6, 0x02d9, 0x02dd, 0x02db, 0x02da, 0x02dc,  
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,  
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,  
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,  
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,  
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,  
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,  
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,  
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,  
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,  
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,  
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,  
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0xfffd,  
    0x2022, 0x2020, 0x2021, 0x2026, 0x2014, 0x2013, 0x0192, 0x2044,  
    0x2039, 0x203a, 0x2212, 0x2030, 0x201e, 0x201c, 0x201d, 0x2018,  
    0x2019, 0x201a, 0x2122, 0xfb01, 0xfb02, 0x0141, 0x0152, 0x0160,  
    0x0178, 0x017d, 0x0131, 0x0142, 0x0153, 0x0161, 0x017e, 0xfffd,  
    0x20ac, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,  
    0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0xfffd, 0x00ae, 0x00af,  
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,  
    0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,  
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,  
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,  
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,  
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,  
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,  
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,  
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,  
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,  
};

typedef struct rect_node_s rect_node;

//typedef enum { false, true } bool;

struct rect_node_s
{
	fz_rect rect;
	rect_node *next;
};

typedef struct
{
	int number;
	int width;
	int height;
	fz_rect media_box;
	fz_page *page;
	rect_node *changed_rects;
	rect_node *hq_changed_rects;
	fz_display_list *page_list;
	fz_display_list *annot_list;
} page_cache;

typedef struct globals_s globals;

struct globals_s
{
	fz_colorspace *colorspace;
	fz_document *doc;
	int resolution;
	fz_context *ctx;
	fz_rect *hit_bbox;
	int current;
	char *current_path;

	page_cache pages[NUM_CACHE];

	int alerts_initialised;
	// fin_lock and fin_lock2 are used during shutdown. The two waiting tasks
	// show_alert and waitForAlertInternal respectively take these locks while
	// waiting. During shutdown, the conditions are signaled and then the fin_locks
	// are taken momentarily to ensure the blocked threads leave the controlled
	// area of code before the mutexes and condition variables are destroyed.
	pthread_mutex_t fin_lock;
	pthread_mutex_t fin_lock2;
	// alert_lock is the main lock guarding the variables directly below.
	pthread_mutex_t alert_lock;
	// Flag indicating if the alert system is active. When not active, both
	// show_alert and waitForAlertInternal return immediately.
	int alerts_active;
	// Pointer to the alert struct passed in by show_alert, and valid while
	// show_alert is blocked.
	pdf_alert_event *current_alert;
	// Flag and condition varibles to signal a request is present and a reply
	// is present, respectively. The condition variables alone are not sufficient
	// because of the pthreads permit spurious signals.
	int alert_request;
	int alert_reply;
	pthread_cond_t alert_request_cond;
	pthread_cond_t alert_reply_cond;

	// For the buffer reading mode, we need to implement stream reading, which
	// needs access to the following.
	JNIEnv *env;
	jclass thiz;

	float inkThickness;
    float inkColor[3];
    float highlightColor[3];
    float underlineColor[3];
    float strikeoutColor[3];
    float textAnnotIconColor[3];
};

static jfieldID global_fid;
static jfieldID buffer_fid;

static void drop_changed_rects(fz_context *ctx, rect_node **nodePtr)
{
	rect_node *node = *nodePtr;
	while (node)
	{
		rect_node *tnode = node;
		node = node->next;
		fz_free(ctx, tnode);
	}

	*nodePtr = NULL;
}

static void drop_page_cache(globals *glo, page_cache *pc)
{
	fz_context *ctx = glo->ctx;
	fz_document *doc = glo->doc;

	LOGI("Drop page %d", pc->number);
	fz_drop_display_list(ctx, pc->page_list);
	pc->page_list = NULL;
	fz_drop_display_list(ctx, pc->annot_list);
	pc->annot_list = NULL;
	fz_drop_page(ctx, pc->page);
	pc->page = NULL;
	drop_changed_rects(ctx, &pc->changed_rects);
	drop_changed_rects(ctx, &pc->hq_changed_rects);
}

static void dump_annotation_display_lists(globals *glo)
{
	fz_context *ctx = glo->ctx;
	int i;

	for (i = 0; i < NUM_CACHE; i++) {
		fz_drop_display_list(ctx, glo->pages[i].annot_list);
		glo->pages[i].annot_list = NULL;
	}
}

static void show_alert(globals *glo, pdf_alert_event *alert)
{
	pthread_mutex_lock(&glo->fin_lock2);
	pthread_mutex_lock(&glo->alert_lock);

	LOGT("Enter show_alert: %s", alert->title);
	alert->button_pressed = 0;

	if (glo->alerts_active)
	{
		glo->current_alert = alert;
		glo->alert_request = 1;
		pthread_cond_signal(&glo->alert_request_cond);

		while (glo->alerts_active && !glo->alert_reply)
			pthread_cond_wait(&glo->alert_reply_cond, &glo->alert_lock);
		glo->alert_reply = 0;
		glo->current_alert = NULL;
	}

	LOGT("Exit show_alert");

	pthread_mutex_unlock(&glo->alert_lock);
	pthread_mutex_unlock(&glo->fin_lock2);
}

static void event_cb(fz_context *ctx, pdf_doc_event *event, void *data)
{
	globals *glo = (globals *)data;

	switch (event->type)
	{
	case PDF_DOCUMENT_EVENT_ALERT:
		show_alert(glo, pdf_access_alert_event(ctx, event));
		break;
	}
}

static void alerts_init(globals *glo)
{
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);

	if (!idoc || glo->alerts_initialised)
		return;

	if (idoc)
		pdf_enable_js(ctx, idoc);

	glo->alerts_active = 0;
	glo->alert_request = 0;
	glo->alert_reply = 0;
	pthread_mutex_init(&glo->fin_lock, NULL);
	pthread_mutex_init(&glo->fin_lock2, NULL);
	pthread_mutex_init(&glo->alert_lock, NULL);
	pthread_cond_init(&glo->alert_request_cond, NULL);
	pthread_cond_init(&glo->alert_reply_cond, NULL);

	pdf_set_doc_event_callback(ctx, idoc, event_cb, glo);
	LOGT("alert_init");
	glo->alerts_initialised = 1;
}

static void alerts_fin(globals *glo)
{
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	if (!glo->alerts_initialised)
		return;

	LOGT("Enter alerts_fin");
	if (idoc)
		pdf_set_doc_event_callback(ctx, idoc, NULL, NULL);

	// Set alerts_active false and wake up show_alert and waitForAlertInternal,
	pthread_mutex_lock(&glo->alert_lock);
	glo->current_alert = NULL;
	glo->alerts_active = 0;
	pthread_cond_signal(&glo->alert_request_cond);
	pthread_cond_signal(&glo->alert_reply_cond);
	pthread_mutex_unlock(&glo->alert_lock);

	// Wait for the fin_locks.
	pthread_mutex_lock(&glo->fin_lock);
	pthread_mutex_unlock(&glo->fin_lock);
	pthread_mutex_lock(&glo->fin_lock2);
	pthread_mutex_unlock(&glo->fin_lock2);

	pthread_cond_destroy(&glo->alert_reply_cond);
	pthread_cond_destroy(&glo->alert_request_cond);
	pthread_mutex_destroy(&glo->alert_lock);
	pthread_mutex_destroy(&glo->fin_lock2);
	pthread_mutex_destroy(&glo->fin_lock);
	LOGT("Exit alerts_fin");
	glo->alerts_initialised = 0;
}

// Should only be called from the single background AsyncTask thread
static globals *get_globals(JNIEnv *env, jobject thiz)
{
	globals *glo = (globals *)(intptr_t)((*env)->GetLongField(env, thiz, global_fid));
	if (glo != NULL)
	{
		glo->env = env;
		glo->thiz = thiz;
	}
	return glo;
}

// May be called from any thread, provided the values of glo->env and glo->thiz
// are not used.
static globals *get_globals_any_thread(JNIEnv *env, jobject thiz)
{
	return (globals *)(intptr_t)((*env)->GetLongField(env, thiz, global_fid));
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	srand(time(NULL));
	return JNI_VERSION_1_2; // return this to make dalvik happy: https://groups.google.com/forum/#!topic/android-ndk/ukQBmKJH2eM
}


JNIEXPORT jlong JNICALL
JNI_FN(MuPDFCore_openFile)(JNIEnv * env, jobject thiz, jstring jfilename)
{
	const char *filename;
	globals *glo;
	fz_context *ctx;
	jclass clazz;

#ifdef NDK_PROFILER
	monstartup("libmupdf.so");
#endif

	clazz = (*env)->GetObjectClass(env, thiz);
	global_fid = (*env)->GetFieldID(env, clazz, "globals", "J");

	glo = calloc(1, sizeof(*glo));
	if (glo == NULL)
		return 0;
	glo->resolution = 160;
	glo->alerts_initialised = 0;

#ifdef DEBUG
	/* Try and send stdout/stderr to file in debug builds. This
	 * path may not work on all platforms, but it works on the
	 * LG G3, and it's no worse than not redirecting it anywhere
	 * on anything else. */
	freopen("/storage/emulated/0/Download/stdout.txt", "a", stdout);
	freopen("/storage/emulated/0/Download/stderr.txt", "a", stderr);
#endif

        //Initialized defaults for annotation styling
    glo->inkThickness = INK_THICKNESS;
    glo->inkColor[0] = INK_COLORr;
    glo->inkColor[1] = INK_COLORg;
    glo->inkColor[2] = INK_COLORb;
    glo->highlightColor[0] = HIGHLIGHT_COLORr;
    glo->highlightColor[1] = HIGHLIGHT_COLORg;
    glo->highlightColor[2] = HIGHLIGHT_COLORb;
    glo->underlineColor[0] = UNDERLINE_COLORr;
    glo->underlineColor[1] = UNDERLINE_COLORg;
    glo->underlineColor[2] = UNDERLINE_COLORb;        
    glo->strikeoutColor[0] = STRIKEOUT_COLORr;
    glo->strikeoutColor[1] = STRIKEOUT_COLORg;
    glo->strikeoutColor[2] = STRIKEOUT_COLORb;
    glo->textAnnotIconColor[0] = TEXTANNOTICON_COLORr;
    glo->textAnnotIconColor[1] = TEXTANNOTICON_COLORg;
    glo->textAnnotIconColor[2] = TEXTANNOTICON_COLORb;
        
    filename = (*env)->GetStringUTFChars(env, jfilename, NULL);
    if (filename == NULL)
    {
        LOGE("Failed to get filename");
        free(glo);
        return 0;
    }

	/* 128 MB store for low memory devices. Tweak as necessary. */
	glo->ctx = ctx = fz_new_context(NULL, NULL, 128 << 20);
	if (!ctx)
	{
		LOGE("Failed to initialise context");
		(*env)->ReleaseStringUTFChars(env, jfilename, filename);
		free(glo);
		return 0;
	}

	fz_register_document_handlers(ctx);

	glo->doc = NULL;
	fz_try(ctx)
	{
		glo->colorspace = fz_device_rgb(ctx);

		LOGI("Opening document...");
		fz_try(ctx)
		{
			glo->current_path = fz_strdup(ctx, (char *)filename);
			glo->doc = fz_open_document(ctx, (char *)filename);
			alerts_init(glo);
		}
		fz_catch(ctx)
		{
			fz_throw(ctx, FZ_ERROR_GENERIC, "Cannot open document: '%s'", filename);
		}
		LOGI("Done!");
	}
	fz_catch(ctx)
	{
		LOGE("Failed: %s", ctx->error->message);
		fz_drop_document(ctx, glo->doc);
		glo->doc = NULL;
		fz_drop_context(ctx);
		glo->ctx = NULL;
		free(glo);
		glo = NULL;
	}

	(*env)->ReleaseStringUTFChars(env, jfilename, filename);

	return (jlong)(intptr_t)glo;
}

typedef struct buffer_state_s
{
	globals *globals;
	char buffer[4096];
}
buffer_state;

static int bufferStreamNext(fz_context *ctx, fz_stream *stream, int max)
{
	buffer_state *bs = (buffer_state *)stream->state;
	globals *glo = bs->globals;
	JNIEnv *env = glo->env;
	jbyteArray array = (jbyteArray)(void *)((*env)->GetObjectField(env, glo->thiz, buffer_fid));
	int arrayLength = (*env)->GetArrayLength(env, array);
	int len = sizeof(bs->buffer);

	if (stream->pos > arrayLength)
		stream->pos = arrayLength;
	if (stream->pos < 0)
		stream->pos = 0;
	if (len + stream->pos > arrayLength)
		len = arrayLength - stream->pos;

	(*env)->GetByteArrayRegion(env, array, stream->pos, len, bs->buffer);
	(*env)->DeleteLocalRef(env, array);

	stream->rp = bs->buffer;
	stream->wp = stream->rp + len;
	stream->pos += len;
	if (len == 0)
		return EOF;
	return *stream->rp++;
}

static void bufferStreamClose(fz_context *ctx, void *state)
{
	fz_free(ctx, state);
}

static void bufferStreamSeek(fz_context *ctx, fz_stream *stream, int offset, int whence)
{
	buffer_state *bs = (buffer_state *)stream->state;
	globals *glo = bs->globals;
	JNIEnv *env = glo->env;
	jbyteArray array = (jbyteArray)(void *)((*env)->GetObjectField(env, glo->thiz, buffer_fid));
	int arrayLength = (*env)->GetArrayLength(env, array);

	(*env)->DeleteLocalRef(env, array);

	if (whence == 0) /* SEEK_SET */
		stream->pos = offset;
	else if (whence == 1) /* SEEK_CUR */
		stream->pos += offset;
	else if (whence == 2) /* SEEK_END */
		stream->pos = arrayLength + offset;

	if (stream->pos > arrayLength)
		stream->pos = arrayLength;
	if (stream->pos < 0)
		stream->pos = 0;

	stream->wp = stream->rp;
}

JNIEXPORT jlong JNICALL
JNI_FN(MuPDFCore_openBuffer)(JNIEnv * env, jobject thiz, jstring jmagic)
{
	globals *glo;
	fz_context *ctx;
	jclass clazz;
	fz_stream *stream = NULL;
	buffer_state *bs;
	const char *magic;

#ifdef NDK_PROFILER
	monstartup("libmupdf.so");
#endif

	clazz = (*env)->GetObjectClass(env, thiz);
	global_fid = (*env)->GetFieldID(env, clazz, "globals", "J");

	glo = calloc(1, sizeof(*glo));
	if (glo == NULL)
		return 0;
	glo->resolution = 160;
	glo->alerts_initialised = 0;
	glo->env = env;
	glo->thiz = thiz;
	buffer_fid = (*env)->GetFieldID(env, clazz, "fileBuffer", "[B");

	magic = (*env)->GetStringUTFChars(env, jmagic, NULL);
	if (magic == NULL)
	{
		LOGE("Failed to get magic");
		free(glo);
		return 0;
	}

	/* 128 MB store for low memory devices. Tweak as necessary. */
	glo->ctx = ctx = fz_new_context(NULL, NULL, 128 << 20);
	if (!ctx)
	{
		LOGE("Failed to initialise context");
		(*env)->ReleaseStringUTFChars(env, jmagic, magic);
		free(glo);
		return 0;
	}

	fz_register_document_handlers(ctx);
	fz_var(stream);

	glo->doc = NULL;
	fz_try(ctx)
	{
		bs = fz_malloc_struct(ctx, buffer_state);
		bs->globals = glo;
		stream = fz_new_stream(ctx, bs, bufferStreamNext, bufferStreamClose);
		stream->seek = bufferStreamSeek;

		glo->colorspace = fz_device_rgb(ctx);

		LOGI("Opening document...");
		fz_try(ctx)
		{
			glo->current_path = NULL;
			glo->doc = fz_open_document_with_stream(ctx, magic, stream);
			alerts_init(glo);
		}
		fz_catch(ctx)
		{
			fz_throw(ctx, FZ_ERROR_GENERIC, "Cannot open memory document");
		}
		LOGI("Done!");
	}
	fz_always(ctx)
	{
		fz_drop_stream(ctx, stream);
	}
	fz_catch(ctx)
	{
		LOGE("Failed: %s", ctx->error->message);
		fz_drop_document(ctx, glo->doc);
		glo->doc = NULL;
		fz_drop_context(ctx);
		glo->ctx = NULL;
		free(glo);
		glo = NULL;
	}

	(*env)->ReleaseStringUTFChars(env, jmagic, magic);

	return (jlong)(intptr_t)glo;
}

JNIEXPORT int JNICALL
JNI_FN(MuPDFCore_countPagesInternal)(JNIEnv *env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	int count = 0;

	fz_try(ctx)
	{
		count = fz_count_pages(ctx, glo->doc);
	}
	fz_catch(ctx)
	{
		LOGE("exception while counting pages: %s", ctx->error->message);
	}
	return count;
}

JNIEXPORT jstring JNICALL
JNI_FN(MuPDFCore_fileFormatInternal)(JNIEnv * env, jobject thiz)
{
	char info[64];
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;

	fz_lookup_metadata(ctx, glo->doc, FZ_META_FORMAT, info, sizeof(info));

	return (*env)->NewStringUTF(env, info);
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_isUnencryptedPDFInternal)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals_any_thread(env, thiz);
	if (glo == NULL)
		return JNI_FALSE;

	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	if (idoc == NULL)
		return JNI_FALSE; // Not a PDF

	int cryptVer = pdf_crypt_version(ctx, idoc);
	return (cryptVer == 0) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_gotoPageInternal)(JNIEnv *env, jobject thiz, int page)
{
	int i;
	int furthest;
	int furthest_dist = -1;
	float zoom;
	fz_matrix ctm;
	fz_irect bbox;
	page_cache *pc;
	globals *glo = get_globals(env, thiz);
	if (glo == NULL)
		return;
	fz_context *ctx = glo->ctx;

	for (i = 0; i < NUM_CACHE; i++)
	{
		if (glo->pages[i].page != NULL && glo->pages[i].number == page)
		{
			/* The page is already cached */
			glo->current = i;
			return;
		}

		if (glo->pages[i].page == NULL)
		{
			/* cache record unused, and so a good one to use */
			furthest = i;
			furthest_dist = INT_MAX;
		}
		else
		{
			int dist = abs(glo->pages[i].number - page);

			/* Further away - less likely to be needed again */
			if (dist > furthest_dist)
			{
				furthest_dist = dist;
				furthest = i;
			}
		}
	}

	glo->current = furthest;
	pc = &glo->pages[glo->current];

	drop_page_cache(glo, pc);

	/* In the event of an error, ensure we give a non-empty page */
	pc->width = 100;
	pc->height = 100;

	pc->number = page;
	LOGI("Goto page %d...", page);
	fz_try(ctx)
	{
		fz_rect rect;
		LOGI("Load page %d", pc->number);
		pc->page = fz_load_page(ctx, glo->doc, pc->number);
		zoom = glo->resolution / 72;
		fz_bound_page(ctx, pc->page, &pc->media_box);
		fz_scale(&ctm, zoom, zoom);
		rect = pc->media_box;
		fz_round_rect(&bbox, fz_transform_rect(&rect, &ctm));
		pc->width = bbox.x1-bbox.x0;
		pc->height = bbox.y1-bbox.y0;
	}
	fz_catch(ctx)
	{
		LOGE("cannot make displaylist from page %d", pc->number);
	}
}

JNIEXPORT float JNICALL
JNI_FN(MuPDFCore_getPageWidth)(JNIEnv *env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	LOGI("PageWidth=%d", glo->pages[glo->current].width);
	return glo->pages[glo->current].width;
}

JNIEXPORT float JNICALL
JNI_FN(MuPDFCore_getPageHeight)(JNIEnv *env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	LOGI("PageHeight=%d", glo->pages[glo->current].height);
	return glo->pages[glo->current].height;
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_javascriptSupported)(JNIEnv *env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	if (idoc)
		return pdf_js_supported(ctx, idoc);
	return 0;
}

static void update_changed_rects(globals *glo, page_cache *pc, pdf_document *idoc)
{
	fz_context *ctx = glo->ctx;
	fz_annot *annot;

	pdf_update_page(ctx, idoc, (pdf_page *)pc->page);
	while ((annot = (fz_annot *)pdf_poll_changed_annot(ctx, idoc, (pdf_page *)pc->page)) != NULL)
	{
		/* FIXME: We bound the annot twice here */
		rect_node *node = fz_malloc_struct(glo->ctx, rect_node);
		fz_bound_annot(ctx, pc->page, annot, &node->rect);
		node->next = pc->changed_rects;
		pc->changed_rects = node;

		node = fz_malloc_struct(glo->ctx, rect_node);
		fz_bound_annot(ctx, pc->page, annot, &node->rect);
		node->next = pc->hq_changed_rects;
		pc->hq_changed_rects = node;
	}
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_drawPage)(JNIEnv *env, jobject thiz, jobject bitmap,
		int pageW, int pageH, int patchX, int patchY, int patchW, int patchH, jlong cookiePtr)
{
	AndroidBitmapInfo info;
	void *pixels;
	int ret;
	fz_device *dev = NULL;
	float zoom;
	fz_matrix ctm;
	fz_irect bbox;
	fz_rect rect;
	fz_pixmap *pix = NULL;
	float xscale, yscale;
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	fz_document *doc = glo->doc;
	page_cache *pc = &glo->pages[glo->current];
	int hq = (patchW < pageW || patchH < pageH);
	fz_matrix scale;
	fz_cookie *cookie = (fz_cookie *)(intptr_t)cookiePtr;

	if (pc->page == NULL)
		return 0;

	fz_var(pix);
	fz_var(dev);

	LOGI("In native method\n");
	if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return 0;
	}

	LOGI("Checking format\n");
	if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
		LOGE("Bitmap format is not RGBA_8888 !");
		return 0;
	}

	LOGI("locking pixels\n");
	if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
		return 0;
	}

	/* Call mupdf to render display list to screen */
	LOGI("Rendering page(%d)=%dx%d patch=[%d,%d,%d,%d]",
			pc->number, pageW, pageH, patchX, patchY, patchW, patchH);

	fz_try(ctx)
	{
		fz_irect pixbbox;
		pdf_document *idoc = pdf_specifics(ctx, doc);

		if (idoc)
		{
			/* Update the changed-rects for both hq patch and main bitmap */
			update_changed_rects(glo, pc, idoc);

			/* Then drop the changed-rects for the bitmap we're about to
			render because we are rendering the entire area */
			drop_changed_rects(ctx, hq ? &pc->hq_changed_rects : &pc->changed_rects);
		}

		if (pc->page_list == NULL)
		{
			/* Render to list */
			pc->page_list = fz_new_display_list(ctx);
			dev = fz_new_list_device(ctx, pc->page_list);

            LOGI("native draw_page() with cookie=%d", (int)cookie);
            if (cookie != NULL && !cookie->abort)
                fz_run_page_contents(ctx, pc->page, dev, &fz_identity, cookie);
			fz_drop_device(ctx, dev);
			dev = NULL;
			if (cookie != NULL && cookie->abort)
			{
				fz_drop_display_list(ctx, pc->page_list);
				pc->page_list = NULL;
				fz_throw(ctx, FZ_ERROR_GENERIC, "Render aborted");
			}
		}
		if (pc->annot_list == NULL)
		{
			fz_annot *annot;
			pc->annot_list = fz_new_display_list(ctx);
			dev = fz_new_list_device(ctx, pc->annot_list);
			for (annot = fz_first_annot(ctx, pc->page); annot; annot = fz_next_annot(ctx, pc->page, annot)) 
            {
                if (cookie == NULL || cookie->abort)
                    break;
				fz_run_annot(ctx, pc->page, annot, dev, &fz_identity, cookie);
            }
			fz_drop_device(ctx, dev);
			dev = NULL;
			if (cookie != NULL && cookie->abort)
			{
				fz_drop_display_list(ctx, pc->annot_list);
				pc->annot_list = NULL;
				fz_throw(ctx, FZ_ERROR_GENERIC, "Render aborted");
			}
		}
		bbox.x0 = patchX;
		bbox.y0 = patchY;
		bbox.x1 = patchX + patchW;
		bbox.y1 = patchY + patchH;
		pixbbox = bbox;
		pixbbox.x1 = pixbbox.x0 + info.width;
		/* pixmaps cannot handle right-edge padding, so the bbox must be expanded to
		 * match the pixels data */
		pix = fz_new_pixmap_with_bbox_and_data(ctx, glo->colorspace, &pixbbox, pixels);
		if (pc->page_list == NULL && pc->annot_list == NULL)
		{
			fz_clear_pixmap_with_value(ctx, pix, 0xd0);
			break;
		}
		fz_clear_pixmap_with_value(ctx, pix, 0xff);

		zoom = glo->resolution / 72;
		fz_scale(&ctm, zoom, zoom);
		rect = pc->media_box;
		fz_round_rect(&bbox, fz_transform_rect(&rect, &ctm));
		/* Now, adjust ctm so that it would give the correct page width
		 * heights. */
		xscale = (float)pageW/(float)(bbox.x1-bbox.x0);
		yscale = (float)pageH/(float)(bbox.y1-bbox.y0);
		fz_concat(&ctm, &ctm, fz_scale(&scale, xscale, yscale));
		rect = pc->media_box;
		fz_transform_rect(&rect, &ctm);
		dev = fz_new_draw_device(ctx, pix);
#ifdef TIME_DISPLAY_LIST
		{
			clock_t time;
			int i;

			LOGI("Executing display list");
			time = clock();
			for (i=0; i<100;i++) {
#endif
				if (pc->page_list)
					fz_run_display_list(ctx, pc->page_list, dev, &ctm, &rect, cookie);
				if (cookie != NULL && cookie->abort)
					fz_throw(ctx, FZ_ERROR_GENERIC, "Render aborted");

				if (pc->annot_list)
					fz_run_display_list(ctx, pc->annot_list, dev, &ctm, &rect, cookie);
				if (cookie != NULL && cookie->abort)
					fz_throw(ctx, FZ_ERROR_GENERIC, "Render aborted");

#ifdef TIME_DISPLAY_LIST
			}
			time = clock() - time;
			LOGI("100 renders in %d (%d per sec)", time, CLOCKS_PER_SEC);
		}
#endif
		fz_drop_device(ctx, dev);
		dev = NULL;
		fz_drop_pixmap(ctx, pix);
		LOGI("Rendered");
	}
	fz_always(ctx)
	{
		fz_drop_device(ctx, dev);
		dev = NULL;
	}
	fz_catch(ctx)
	{
		LOGE("Render failed");
	}

	AndroidBitmap_unlockPixels(env, bitmap);

	return 1;
}

static char *widget_type_string(int t)
{
	switch(t)
	{
	case PDF_WIDGET_TYPE_PUSHBUTTON: return "pushbutton";
	case PDF_WIDGET_TYPE_CHECKBOX: return "checkbox";
	case PDF_WIDGET_TYPE_RADIOBUTTON: return "radiobutton";
	case PDF_WIDGET_TYPE_TEXT: return "text";
	case PDF_WIDGET_TYPE_LISTBOX: return "listbox";
	case PDF_WIDGET_TYPE_COMBOBOX: return "combobox";
	case PDF_WIDGET_TYPE_SIGNATURE: return "signature";
	default: return "non-widget";
	}
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_updatePageInternal)(JNIEnv *env, jobject thiz, jobject bitmap, int page,
		int pageW, int pageH, int patchX, int patchY, int patchW, int patchH, jlong cookiePtr)
{
	AndroidBitmapInfo info;
	void *pixels;
	int ret;
	fz_device *dev = NULL;
	float zoom;
	fz_matrix ctm;
	fz_irect bbox;
	fz_rect rect;
	fz_pixmap *pix = NULL;
	float xscale, yscale;
	pdf_document *idoc;
	page_cache *pc = NULL;
	int hq = (patchW < pageW || patchH < pageH);
	int i;
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	fz_document *doc = glo->doc;
	rect_node *crect;
	fz_matrix scale;
	fz_cookie *cookie = (fz_cookie *)(intptr_t)cookiePtr;

	for (i = 0; i < NUM_CACHE; i++)
	{
		if (glo->pages[i].page != NULL && glo->pages[i].number == page)
		{
			pc = &glo->pages[i];
			break;
		}
	}

	if (pc == NULL)
	{
		/* Without a cached page object we cannot perform a partial update so
		render the entire bitmap instead */
		JNI_FN(MuPDFCore_gotoPageInternal)(env, thiz, page);
		return JNI_FN(MuPDFCore_drawPage)(env, thiz, bitmap, pageW, pageH, patchX, patchY, patchW, patchH, (jlong)(intptr_t)cookie);
	}

	idoc = pdf_specifics(ctx, doc);

	fz_var(pix);
	fz_var(dev);

	LOGI("In native method\n");
	if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return 0;
	}

	LOGI("Checking format\n");
	if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
		LOGE("Bitmap format is not RGBA_8888 !");
		return 0;
	}

	LOGI("locking pixels\n");
	if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
		return 0;
	}

	/* Call mupdf to render display list to screen */
	LOGI("Rendering page(%d)=%dx%d patch=[%d,%d,%d,%d]",
			pc->number, pageW, pageH, patchX, patchY, patchW, patchH);

	fz_try(ctx)
	{
		fz_annot *annot;
		fz_irect pixbbox;

		if (idoc)
		{
			/* Update the changed-rects for both hq patch and main bitmap */
			update_changed_rects(glo, pc, idoc);
		}

		if (pc->page_list == NULL)
		{
			/* Render to list */
			pc->page_list = fz_new_display_list(ctx);
			dev = fz_new_list_device(ctx, pc->page_list);
			fz_run_page_contents(ctx, pc->page, dev, &fz_identity, cookie);
			fz_drop_device(ctx, dev);
			dev = NULL;
			if (cookie != NULL && cookie->abort)
			{
				fz_drop_display_list(ctx, pc->page_list);
				pc->page_list = NULL;
				fz_throw(ctx, FZ_ERROR_GENERIC, "Render aborted");
			}
		}

		if (pc->annot_list == NULL) {
			pc->annot_list = fz_new_display_list(ctx);
			dev = fz_new_list_device(ctx, pc->annot_list);
			for (annot = fz_first_annot(ctx, pc->page); annot; annot = fz_next_annot(ctx, pc->page, annot))
				fz_run_annot(ctx, pc->page, annot, dev, &fz_identity, cookie);
			fz_drop_device(ctx, dev);
			dev = NULL;
			if (cookie != NULL && cookie->abort)
			{
				fz_drop_display_list(ctx, pc->annot_list);
				pc->annot_list = NULL;
				fz_throw(ctx, FZ_ERROR_GENERIC, "Render aborted");
			}
		}

		bbox.x0 = patchX;
		bbox.y0 = patchY;
		bbox.x1 = patchX + patchW;
		bbox.y1 = patchY + patchH;
		pixbbox = bbox;
		pixbbox.x1 = pixbbox.x0 + info.width;
		/* pixmaps cannot handle right-edge padding, so the bbox must be expanded to
		 * match the pixels data */
		pix = fz_new_pixmap_with_bbox_and_data(ctx, glo->colorspace, &pixbbox, pixels);

		zoom = glo->resolution / 72;
		fz_scale(&ctm, zoom, zoom);
		rect = pc->media_box;
		fz_round_rect(&bbox, fz_transform_rect(&rect, &ctm));
		/* Now, adjust ctm so that it would give the correct page width
		 * heights. */
		xscale = (float)pageW/(float)(bbox.x1-bbox.x0);
		yscale = (float)pageH/(float)(bbox.y1-bbox.y0);
		fz_concat(&ctm, &ctm, fz_scale(&scale, xscale, yscale));
		rect = pc->media_box;
		fz_transform_rect(&rect, &ctm);

		LOGI("Start partial update");
		for (crect = hq ? pc->hq_changed_rects : pc->changed_rects; crect; crect = crect->next)
		{
			fz_irect abox;
			fz_rect arect = crect->rect;
			fz_intersect_rect(fz_transform_rect(&arect, &ctm), &rect);
			fz_round_rect(&abox, &arect);

			LOGI("Update rectangle (%d, %d, %d, %d)", abox.x0, abox.y0, abox.x1, abox.y1);
			if (!fz_is_empty_irect(&abox))
			{
				LOGI("And it isn't empty");
				fz_clear_pixmap_rect_with_value(ctx, pix, 0xff, &abox);
				dev = fz_new_draw_device_with_bbox(ctx, pix, &abox);
				if (pc->page_list)
					fz_run_display_list(ctx, pc->page_list, dev, &ctm, &arect, cookie);
				if (cookie != NULL && cookie->abort)
					fz_throw(ctx, FZ_ERROR_GENERIC, "Render aborted");

				if (pc->annot_list)
					fz_run_display_list(ctx, pc->annot_list, dev, &ctm, &arect, cookie);
				if (cookie != NULL && cookie->abort)
					fz_throw(ctx, FZ_ERROR_GENERIC, "Render aborted");

				fz_drop_device(ctx, dev);
				dev = NULL;
			}
		}
		LOGI("End partial update");

		/* Drop the changed rects we've just rendered */
		drop_changed_rects(ctx, hq ? &pc->hq_changed_rects : &pc->changed_rects);

		LOGI("Rendered");
	}
	fz_always(ctx)
	{
		fz_drop_device(ctx, dev);
		dev = NULL;
	}
	fz_catch(ctx)
	{
		LOGE("Render failed");
	}

	fz_drop_pixmap(ctx, pix);
	AndroidBitmap_unlockPixels(env, bitmap);

	return 1;
}

static int
charat(fz_context *ctx, fz_text_page *page, int idx)
{
	fz_char_and_box cab;
	return fz_text_char_at(ctx, &cab, page, idx)->c;
}

static fz_rect
bboxcharat(fz_context *ctx, fz_text_page *page, int idx)
{
	fz_char_and_box cab;
	return fz_text_char_at(ctx, &cab, page, idx)->bbox;
}

static int
textlen(fz_text_page *page)
{
	int len = 0;
	int block_num;

	for (block_num = 0; block_num < page->len; block_num++)
	{
		fz_text_block *block;
		fz_text_line *line;

		if (page->blocks[block_num].type != FZ_PAGE_BLOCK_TEXT)
			continue;
		block = page->blocks[block_num].u.text;
		for (line = block->lines; line < block->lines + block->len; line++)
		{
			fz_text_span *span;

			for (span = line->first_span; span; span = span->next)
			{
				len += span->len;
			}
			len++; /* pseudo-newline */
		}
	}
	return len;
}

static int
countOutlineItems(fz_outline *outline)
{
	int count = 0;

	while (outline)
	{
		if (outline->dest.kind == FZ_LINK_GOTO
				&& outline->dest.ld.gotor.page >= 0
				&& outline->title)
			count++;

		count += countOutlineItems(outline->down);
		outline = outline->next;
	}

	return count;
}

static int
fillInOutlineItems(JNIEnv * env, jclass olClass, jmethodID ctor, jobjectArray arr, int pos, fz_outline *outline, int level)
{
	while (outline)
	{
		if (outline->dest.kind == FZ_LINK_GOTO)
		{
			int page = outline->dest.ld.gotor.page;
			if (page >= 0 && outline->title)
			{
				jobject ol;
				jstring title = (*env)->NewStringUTF(env, outline->title);
				if (title == NULL) return -1;
				ol = (*env)->NewObject(env, olClass, ctor, level, title, page);
				if (ol == NULL) return -1;
				(*env)->SetObjectArrayElement(env, arr, pos, ol);
				(*env)->DeleteLocalRef(env, ol);
				(*env)->DeleteLocalRef(env, title);
				pos++;
			}
		}
		pos = fillInOutlineItems(env, olClass, ctor, arr, pos, outline->down, level+1);
		if (pos < 0) return -1;
		outline = outline->next;
	}

	return pos;
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_needsPasswordInternal)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;

	return fz_needs_password(ctx, glo->doc) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_authenticatePasswordInternal)(JNIEnv *env, jobject thiz, jstring password)
{
	const char *pw;
	int result;
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;

	pw = (*env)->GetStringUTFChars(env, password, NULL);
	if (pw == NULL)
		return JNI_FALSE;

	result = fz_authenticate_password(ctx, glo->doc, (char *)pw);
	(*env)->ReleaseStringUTFChars(env, password, pw);
	return result;
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_hasOutlineInternal)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	fz_outline *outline = fz_load_outline(ctx, glo->doc);

	fz_drop_outline(glo->ctx, outline);
	return (outline == NULL) ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_getOutlineInternal)(JNIEnv * env, jobject thiz)
{
	jclass olClass;
	jmethodID ctor;
	jobjectArray arr;
	jobject ol;
	fz_outline *outline;
	int nItems;
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	jobjectArray ret;

	olClass = (*env)->FindClass(env, PACKAGENAME "/OutlineItem");
	if (olClass == NULL) return NULL;
	ctor = (*env)->GetMethodID(env, olClass, "<init>", "(ILjava/lang/String;I)V");
	if (ctor == NULL) return NULL;

	outline = fz_load_outline(ctx, glo->doc);
	nItems = countOutlineItems(outline);

	arr = (*env)->NewObjectArray(env,
					nItems,
					olClass,
					NULL);
	if (arr == NULL) return NULL;

	ret = fillInOutlineItems(env, olClass, ctor, arr, 0, outline, 0) > 0
			? arr
			:NULL;
	fz_drop_outline(glo->ctx, outline);
	return ret;
}

JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_searchPage)(JNIEnv * env, jobject thiz, jstring jtext)
{
	jclass rectClass;
	jmethodID ctor;
	jobjectArray arr;
	jobject rect;
	fz_text_sheet *sheet = NULL;
	fz_text_page *text = NULL;
	fz_device *dev = NULL;
	float zoom;
	fz_matrix ctm;
	int pos;
	int len;
	int i, n;
	int hit_count = 0;
	const char *str;
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	fz_document *doc = glo->doc;
	page_cache *pc = &glo->pages[glo->current];

	rectClass = (*env)->FindClass(env, "android/graphics/RectF");
	if (rectClass == NULL) return NULL;
	ctor = (*env)->GetMethodID(env, rectClass, "<init>", "(FFFF)V");
	if (ctor == NULL) return NULL;
	str = (*env)->GetStringUTFChars(env, jtext, NULL);
	if (str == NULL) return NULL;

	fz_var(sheet);
	fz_var(text);
	fz_var(dev);

	fz_try(ctx)
	{
		if (glo->hit_bbox == NULL)
			glo->hit_bbox = fz_malloc_array(ctx, MAX_SEARCH_HITS, sizeof(*glo->hit_bbox));

		zoom = glo->resolution / 72;
		fz_scale(&ctm, zoom, zoom);
		sheet = fz_new_text_sheet(ctx);
		text = fz_new_text_page(ctx);
		dev = fz_new_text_device(ctx, sheet, text);
		fz_run_page(ctx, pc->page, dev, &ctm, NULL);
		fz_drop_device(ctx, dev);
		dev = NULL;

		hit_count = fz_search_text_page(ctx, text, str, glo->hit_bbox, MAX_SEARCH_HITS);
	}
	fz_always(ctx)
	{
		fz_drop_text_page(ctx, text);
		fz_drop_text_sheet(ctx, sheet);
		fz_drop_device(ctx, dev);
	}
	fz_catch(ctx)
	{
		jclass cls;
		(*env)->ReleaseStringUTFChars(env, jtext, str);
		cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (cls != NULL)
			(*env)->ThrowNew(env, cls, "Out of memory in MuPDFCore_searchPage");
		(*env)->DeleteLocalRef(env, cls);

		return NULL;
	}

	(*env)->ReleaseStringUTFChars(env, jtext, str);

	arr = (*env)->NewObjectArray(env,
					hit_count,
					rectClass,
					NULL);
	if (arr == NULL) return NULL;

	for (i = 0; i < hit_count; i++) {
		rect = (*env)->NewObject(env, rectClass, ctor,
				(float) (glo->hit_bbox[i].x0),
				(float) (glo->hit_bbox[i].y0),
				(float) (glo->hit_bbox[i].x1),
				(float) (glo->hit_bbox[i].y1));
		if (rect == NULL)
			return NULL;
		(*env)->SetObjectArrayElement(env, arr, i, rect);
		(*env)->DeleteLocalRef(env, rect);
	}

	return arr;
}

JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_text)(JNIEnv * env, jobject thiz)
{
	jclass textCharClass;
	jclass textSpanClass;
	jclass textLineClass;
	jclass textBlockClass;
	jmethodID ctor;
	jobjectArray barr = NULL;
	fz_text_sheet *sheet = NULL;
	fz_text_page *text = NULL;
	fz_device *dev = NULL;
	float zoom;
	fz_matrix ctm;
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	fz_document *doc = glo->doc;
	page_cache *pc = &glo->pages[glo->current];

	textCharClass = (*env)->FindClass(env, PACKAGENAME "/TextChar");
	if (textCharClass == NULL) return NULL;
	textSpanClass = (*env)->FindClass(env, "[L" PACKAGENAME "/TextChar;");
	if (textSpanClass == NULL) return NULL;
	textLineClass = (*env)->FindClass(env, "[[L" PACKAGENAME "/TextChar;");
	if (textLineClass == NULL) return NULL;
	textBlockClass = (*env)->FindClass(env, "[[[L" PACKAGENAME "/TextChar;");
	if (textBlockClass == NULL) return NULL;
	ctor = (*env)->GetMethodID(env, textCharClass, "<init>", "(FFFFC)V");
	if (ctor == NULL) return NULL;

	fz_var(sheet);
	fz_var(text);
	fz_var(dev);

	fz_try(ctx)
	{
		int b, l, s, c;

		zoom = glo->resolution / 72;
		fz_scale(&ctm, zoom, zoom);
		sheet = fz_new_text_sheet(ctx);
		text = fz_new_text_page(ctx);
		dev = fz_new_text_device(ctx, sheet, text);
		fz_run_page(ctx, pc->page, dev, &ctm, NULL);
		fz_drop_device(ctx, dev);
		dev = NULL;

		barr = (*env)->NewObjectArray(env, text->len, textBlockClass, NULL);
		if (barr == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "NewObjectArray failed");

		for (b = 0; b < text->len; b++)
		{
			fz_text_block *block;
			jobjectArray *larr;

			if (text->blocks[b].type != FZ_PAGE_BLOCK_TEXT)
				continue;
			block = text->blocks[b].u.text;
			larr = (*env)->NewObjectArray(env, block->len, textLineClass, NULL);
			if (larr == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "NewObjectArray failed");

			for (l = 0; l < block->len; l++)
			{
				fz_text_line *line = &block->lines[l];
				jobjectArray *sarr;
				fz_text_span *span;
				int len = 0;

				for (span = line->first_span; span; span = span->next)
					len++;

				sarr = (*env)->NewObjectArray(env, len, textSpanClass, NULL);
				if (sarr == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "NewObjectArray failed");

				for (s=0, span = line->first_span; span; s++, span = span->next)
				{
					jobjectArray *carr = (*env)->NewObjectArray(env, span->len, textCharClass, NULL);
					if (carr == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "NewObjectArray failed");

					for (c = 0; c < span->len; c++)
					{
						fz_text_char *ch = &span->text[c];
						fz_rect bbox;
						fz_text_char_bbox(ctx, &bbox, span, c);
						jobject cobj = (*env)->NewObject(env, textCharClass, ctor, bbox.x0, bbox.y0, bbox.x1, bbox.y1, ch->c);
						if (cobj == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "NewObjectfailed");

						(*env)->SetObjectArrayElement(env, carr, c, cobj);
						(*env)->DeleteLocalRef(env, cobj);
					}

					(*env)->SetObjectArrayElement(env, sarr, s, carr);
					(*env)->DeleteLocalRef(env, carr);
				}

				(*env)->SetObjectArrayElement(env, larr, l, sarr);
				(*env)->DeleteLocalRef(env, sarr);
			}

			(*env)->SetObjectArrayElement(env, barr, b, larr);
			(*env)->DeleteLocalRef(env, larr);
		}
	}
	fz_always(ctx)
	{
		fz_drop_text_page(ctx, text);
		fz_drop_text_sheet(ctx, sheet);
		fz_drop_device(ctx, dev);
	}
	fz_catch(ctx)
	{
		jclass cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (cls != NULL)
			(*env)->ThrowNew(env, cls, "Out of memory in MuPDFCore_text");
		(*env)->DeleteLocalRef(env, cls);

		return NULL;
	}

	return barr;
}

JNIEXPORT jbyteArray JNICALL
JNI_FN(MuPDFCore_textAsHtml)(JNIEnv * env, jobject thiz)
{
	fz_text_sheet *sheet = NULL;
	fz_text_page *text = NULL;
	fz_device *dev = NULL;
	fz_matrix ctm;
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	fz_document *doc = glo->doc;
	page_cache *pc = &glo->pages[glo->current];
	jbyteArray bArray = NULL;
	fz_buffer *buf = NULL;
	fz_output *out = NULL;

	fz_var(sheet);
	fz_var(text);
	fz_var(dev);
	fz_var(buf);
	fz_var(out);

	fz_try(ctx)
	{
		int b, l, s, c;

		ctm = fz_identity;
		sheet = fz_new_text_sheet(ctx);
		text = fz_new_text_page(ctx);
		dev = fz_new_text_device(ctx, sheet, text);
		fz_run_page(ctx, pc->page, dev, &ctm, NULL);
		fz_drop_device(ctx, dev);
		dev = NULL;

		fz_analyze_text(ctx, sheet, text);

		buf = fz_new_buffer(ctx, 256);
		out = fz_new_output_with_buffer(ctx, buf);
		fz_printf(ctx, out, "<html>\n");
		fz_printf(ctx, out, "<style>\n");
		fz_printf(ctx, out, "body{margin:0;}\n");
		fz_printf(ctx, out, "div.page{background-color:white;}\n");
		fz_printf(ctx, out, "div.block{margin:0pt;padding:0pt;}\n");
		fz_printf(ctx, out, "div.metaline{display:table;width:100%%}\n");
		fz_printf(ctx, out, "div.line{display:table-row;}\n");
		fz_printf(ctx, out, "div.cell{display:table-cell;padding-left:0.25em;padding-right:0.25em}\n");
		//fz_printf(ctx, out, "p{margin:0;padding:0;}\n");
		fz_printf(ctx, out, "</style>\n");
		fz_printf(ctx, out, "<body style=\"margin:0\"><div style=\"padding:10px\" id=\"content\">");
		fz_print_text_page_html(ctx, out, text);
		fz_printf(ctx, out, "</div></body>\n");
		fz_printf(ctx, out, "<style>\n");
		fz_print_text_sheet(ctx, out, sheet);
		fz_printf(ctx, out, "</style>\n</html>\n");
		fz_drop_output(ctx, out);
		out = NULL;

		bArray = (*env)->NewByteArray(env, buf->len);
		if (bArray == NULL)
			fz_throw(ctx, FZ_ERROR_GENERIC, "Failed to make byteArray");
		(*env)->SetByteArrayRegion(env, bArray, 0, buf->len, buf->data);

	}
	fz_always(ctx)
	{
		fz_drop_text_page(ctx, text);
		fz_drop_text_sheet(ctx, sheet);
		fz_drop_device(ctx, dev);
		fz_drop_output(ctx, out);
		fz_drop_buffer(ctx, buf);
	}
	fz_catch(ctx)
	{
		jclass cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
		if (cls != NULL)
			(*env)->ThrowNew(env, cls, "Out of memory in MuPDFCore_textAsHtml");
		(*env)->DeleteLocalRef(env, cls);

		return NULL;
	}

	return bArray;
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_addMarkupAnnotationInternal)(JNIEnv * env, jobject thiz, jobjectArray points, fz_annot_type type, jstring jtext)
{
    globals *glo = get_globals(env, thiz);
    if (glo == NULL) return;
    fz_context *ctx = glo->ctx;
    fz_document *doc = glo->doc;
    pdf_document *idoc = pdf_specifics(ctx, doc);
    page_cache *pc = &glo->pages[glo->current];
    jclass pt_cls;
    jfieldID x_fid, y_fid;
    int i, n;
    fz_point *pts = NULL;
    float color[3];
    float alpha;
    float line_height;
    float line_thickness;
    
    if (idoc == NULL)
        return;    
            
    switch (type)
    {
        case FZ_ANNOT_HIGHLIGHT:
            color[0] = glo->highlightColor[0];
            color[1] = glo->highlightColor[1];
            color[2] = glo->highlightColor[2];
            alpha = 0.69f; //HACK: Alphas smaller than 0.7 also get /BM Multiply in pdf_dev_alpha and so are displayed "behind" the text!
            line_thickness = 1.0;
            line_height = 0.5;
            break;
        case FZ_ANNOT_UNDERLINE:
            color[0] = glo->underlineColor[0];
            color[1] = glo->underlineColor[1];
            color[2] = glo->underlineColor[2];
            alpha = 1.0f;
            line_thickness = LINE_THICKNESS;
            line_height = UNDERLINE_HEIGHT;
            break;
        case FZ_ANNOT_STRIKEOUT:
            color[0] = glo->strikeoutColor[0];
            color[1] = glo->strikeoutColor[1];
            color[2] = glo->strikeoutColor[2];
            alpha = 1.0f;
            line_thickness = LINE_THICKNESS;
            line_height = STRIKE_HEIGHT;
            break;
        case FZ_ANNOT_TEXT:
            color[0] = glo->textAnnotIconColor[0];
            color[1] = glo->textAnnotIconColor[1];
            color[2] = glo->textAnnotIconColor[2];
            alpha = 1.0f;
            break;
        default:
            return;
    }

    fz_var(pts);
    fz_try(ctx)
    {
        fz_annot *annot;
        fz_matrix ctm;

        float zoom = glo->resolution / 72;
        zoom = 1.0 / zoom;
        fz_scale(&ctm, zoom, zoom);
//        pt_cls = (*env)->FindClass(env, "android.graphics.PointF");
        pt_cls = (*env)->FindClass(env, "android/graphics/PointF");
        if (pt_cls == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "FindClass");
        x_fid = (*env)->GetFieldID(env, pt_cls, "x", "F");
        if (x_fid == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "GetFieldID(x)");
        y_fid = (*env)->GetFieldID(env, pt_cls, "y", "F");
        if (y_fid == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "GetFieldID(y)");

        n = (*env)->GetArrayLength(env, points);

        pts = fz_malloc_array(ctx, n, sizeof(fz_point));

        for (i = 0; i < n; i++)
        {
                //Fix the order of the points in the quad points of highlight annotations
            jobject opt;
            if(type == FZ_ANNOT_HIGHLIGHT)
            {
                if(i%4 == 2)
                    opt = (*env)->GetObjectArrayElement(env, points, i+1);
                else if(i%4 == 3)
                    opt = (*env)->GetObjectArrayElement(env, points, i-1);
                else
                    opt = (*env)->GetObjectArrayElement(env, points, i);
            }
            else
                opt = (*env)->GetObjectArrayElement(env, points, i);
            
            pts[i].x = opt ? (*env)->GetFloatField(env, opt, x_fid) : 0.0f;
            pts[i].y = opt ? (*env)->GetFloatField(env, opt, y_fid) : 0.0f;
            fz_transform_point(&pts[i], &ctm);
        }

        annot = (fz_annot *)pdf_create_annot(ctx, idoc, (pdf_page *)pc->page, type); //in pdf-annot-edit.c creates a simple annot without AP (alpha is not honored here for example but /BM is set for highlihgt annotations)

            //Now we generate the AP:
        if(type == FZ_ANNOT_TEXT)
        {
                //Ensure order of points
            if(pts[0].x > pts[1].x)
            {
                float z = pts[1].x;
                pts[1].x = pts[0].x;
                pts[0].x = z;
            }
            if(pts[0].y > pts[1].y)
            {
                float z = pts[1].y;
                pts[1].y = pts[0].y;
                pts[0].y = z;
            }
            
            
            fz_rect rect = {pts[0].x, pts[0].y, pts[1].x, pts[1].y};

            const jchar * text = (*env)->GetStringChars(env, jtext, NULL);
            unsigned int length = (*env)->GetStringLength(env, jtext);

               //Add the BOM to make clear this is UTF-16BE encoding (this is what we get from the java side)
           jchar *dstptr = (jchar *)malloc((length+1)*sizeof(jchar));
           dstptr[0] = 0xfeff;
           for (i = 0; i < length; i++)
               dstptr[i+1] = text[i];

           int i;
           for (i=0; i< length+1; i++)
                   //LOGI("mupdf.c: raw chars of new annotation: %x", dstptr[i]);
           
//            pdf_set_text_details(idoc, (pdf_annot *)annot, &rect, text, length); //in pdf-annot.c
           pdf_set_text_details(ctx, idoc, (pdf_annot *)annot, &rect, dstptr, length+1); //in pdf-annot.c
           
               //Generate an appearance stream (AP) for the annotation (this should only be done once for each document and then the relevant xobject just referenced...)
           const float linewidth = (pts[1].x - pts[0].x)*0.06;
           const fz_matrix *page_ctm = &((pdf_annot *)annot)->page->ctm;
           fz_display_list *dlist = NULL;
           fz_device *dev = NULL;
           fz_path *path = NULL;
           fz_stroke_state *stroke = NULL;
           
           fz_var(path);
           fz_var(stroke);
           fz_var(dev);
           fz_var(dlist);
           fz_try(ctx)
           {
               dlist = fz_new_display_list(ctx);
               dev = fz_new_list_device(ctx, dlist);

               stroke = fz_new_stroke_state(ctx);
               stroke->linewidth = linewidth;
               const float halflinewidth = linewidth*0.5;
               path = fz_new_path(ctx);

               fz_moveto(ctx, path, pts[0].x, pts[1].y-halflinewidth);
               fz_lineto(ctx, path, pts[1].x-halflinewidth, pts[1].y-halflinewidth);
               fz_lineto(ctx, path, pts[1].x-halflinewidth, 0.8*pts[0].y+0.2*pts[1].y);
               fz_lineto(ctx, path, 0.3*pts[1].x+0.7*pts[0].x, 0.8*pts[0].y+0.2*pts[1].y);
               fz_lineto(ctx, path, pts[0].x+halflinewidth, pts[0].y+halflinewidth);
               fz_lineto(ctx, path, pts[0].x+halflinewidth, pts[1].y);

               
               fz_moveto(ctx, path, 0.8*pts[0].x+0.2*pts[1].x, 0.8*pts[1].y+0.2*pts[0].y-halflinewidth);
               fz_lineto(ctx, path, 0.2*pts[0].x+0.8*pts[1].x, 0.8*pts[1].y+0.2*pts[0].y-halflinewidth);
               fz_moveto(ctx, path, 0.8*pts[0].x+0.2*pts[1].x, 0.6*pts[1].y+0.4*pts[0].y);
               fz_lineto(ctx, path, 0.2*pts[0].x+0.8*pts[1].x, 0.6*pts[1].y+0.4*pts[0].y);
               fz_moveto(ctx, path, 0.8*pts[0].x+0.2*pts[1].x, 0.4*pts[1].y+0.6*pts[0].y+halflinewidth);
               fz_lineto(ctx, path, 0.4*pts[0].x+0.6*pts[1].x, 0.4*pts[1].y+0.6*pts[0].y+halflinewidth);
               
               fz_stroke_path(ctx, dev, path, stroke, page_ctm, fz_device_rgb(ctx), color, alpha);
               fz_transform_rect(&rect, page_ctm);
               
               pdf_set_annot_appearance(ctx, idoc, (pdf_annot *)annot, &rect, dlist);
           }
           fz_always(ctx)
           {
               fz_drop_device(ctx, dev);
               fz_drop_display_list(ctx, dlist);
               fz_drop_stroke_state(ctx, stroke);
               fz_drop_path(ctx, path);

               free(dstptr);
               (*env)->ReleaseStringChars(env, jtext, text);
           }
           fz_catch(ctx)
           {
               fz_rethrow(ctx);
           }
        } //Add a markup annotation
        else
        {
            pdf_set_markup_annot_quadpoints(ctx, idoc, (pdf_annot *)annot, pts, n); //in pdf-annot.c
            
            if(type == FZ_ANNOT_HIGHLIGHT) {
                pdf_set_markup_appearance_highlight(ctx, idoc, (pdf_annot *)annot, color, alpha, line_thickness, line_height); //in pdf-appearance.c
            }
            else
            pdf_set_markup_appearance(ctx, idoc, (pdf_annot *)annot, color, alpha, line_thickness, line_height); //in pdf-appearance.c
        }
        
        dump_annotation_display_lists(glo);
    }
    fz_always(ctx)
    {
        fz_free(ctx, pts);
    }
    fz_catch(ctx)
    {
        LOGE("addMarkupAnnotationInternal: %s failed", ctx->error->message);
        jclass cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
        if (cls != NULL)
            (*env)->ThrowNew(env, cls, "Out of memory in MuPDFCore_searchPage");
        (*env)->DeleteLocalRef(env, cls);
    }
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_addInkAnnotationInternal)(JNIEnv * env, jobject thiz, jobjectArray arcs)
{
    globals *glo = get_globals(env, thiz);
    if (glo == NULL) return;
    fz_context *ctx = glo->ctx;
    fz_document *doc = glo->doc;
    pdf_document *idoc = pdf_specifics(ctx, doc);
    page_cache *pc = &glo->pages[glo->current];
    jclass pt_cls;
    jfieldID x_fid, y_fid;
    int i, j, k, n;
    fz_point *pts = NULL;
    int *counts = NULL;
    int total = 0;
    float color[3];

    if (idoc == NULL)
        return;

    color[0] = glo->inkColor[0];
    color[1] = glo->inkColor[1];
    color[2] = glo->inkColor[2];

    fz_var(pts);
    fz_var(counts);
    fz_try(ctx)
    {
        fz_annot *annot;
        fz_matrix ctm;

        float zoom = glo->resolution / 72;
        zoom = 1.0 / zoom;
        fz_scale(&ctm, zoom, zoom);
        pt_cls = (*env)->FindClass(env, "android/graphics/PointF");
        if (pt_cls == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "FindClass");
        x_fid = (*env)->GetFieldID(env, pt_cls, "x", "F");
        if (x_fid == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "GetFieldID(x)");
        y_fid = (*env)->GetFieldID(env, pt_cls, "y", "F");
        if (y_fid == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "GetFieldID(y)");

        n = (*env)->GetArrayLength(env, arcs);

        counts = fz_malloc_array(ctx, n, sizeof(int));

        for (i = 0; i < n; i++)
        {
            jobjectArray arc = (jobjectArray)(*env)->GetObjectArrayElement(env, arcs, i);
            int count = (*env)->GetArrayLength(env, arc);

            counts[i] = count;
            total += count;
        }

        pts = fz_malloc_array(ctx, total, sizeof(fz_point));

        k = 0;
        for (i = 0; i < n; i++)
        {
            jobjectArray arc = (jobjectArray)(*env)->GetObjectArrayElement(env, arcs, i);
            int count = counts[i];

            for (j = 0; j < count; j++)
            {
                jobject pt = (*env)->GetObjectArrayElement(env, arc, j);

                pts[k].x = pt ? (*env)->GetFloatField(env, pt, x_fid) : 0.0f;
                pts[k].y = pt ? (*env)->GetFloatField(env, pt, y_fid) : 0.0f;
                (*env)->DeleteLocalRef(env, pt);
                fz_transform_point(&pts[k], &ctm);
                k++;
            }
            (*env)->DeleteLocalRef(env, arc);
        }

        annot = (fz_annot *)pdf_create_annot(ctx, idoc, (pdf_page *)pc->page, FZ_ANNOT_INK);

        pdf_set_ink_annot_list(ctx, idoc, (pdf_annot *)annot, pts, counts, n, color, glo->inkThickness);

        dump_annotation_display_lists(glo);
    }
    fz_always(ctx)
    {
        fz_free(ctx, pts);
        fz_free(ctx, counts);
    }
    fz_catch(ctx)
    {
        LOGE("addInkAnnotation: %s failed", ctx->error->message);
        jclass cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
        if (cls != NULL)
            (*env)->ThrowNew(env, cls, "Out of memory in MuPDFCore_searchPage");
        (*env)->DeleteLocalRef(env, cls);
    }
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_deleteAnnotationInternal)(JNIEnv * env, jobject thiz, int annot_index)
{
	globals *glo = get_globals(env, thiz);
	if (glo == NULL) return;
	fz_context *ctx = glo->ctx;
	fz_document *doc = glo->doc;
	pdf_document *idoc = pdf_specifics(ctx, doc);
	page_cache *pc = &glo->pages[glo->current];
	fz_annot *annot;
	int i;

	if (idoc == NULL)
		return;

	fz_try(ctx)
	{
		annot = fz_first_annot(ctx, pc->page);
		for (i = 0; i < annot_index && annot; i++)
			annot = fz_next_annot(ctx, pc->page, annot);

		if (annot)
		{
			pdf_delete_annot(ctx, idoc, (pdf_page *)pc->page, (pdf_annot *)annot);
			dump_annotation_display_lists(glo);
		}
	}
	fz_catch(ctx)
	{
		LOGE("deleteAnnotationInternal: %s", ctx->error->message);
	}
}

/* Close the document, at least enough to be able to save over it. This
 * may be called again later, so must be idempotent. */
static void close_doc(globals *glo)
{
	int i;

	fz_free(glo->ctx, glo->hit_bbox);
	glo->hit_bbox = NULL;

	for (i = 0; i < NUM_CACHE; i++)
		drop_page_cache(glo, &glo->pages[i]);

	alerts_fin(glo);

	fz_drop_document(glo->ctx, glo->doc);
	glo->doc = NULL;
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_destroying)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);

	if (glo == NULL)
		return;
	LOGI("Destroying");
	fz_free(glo->ctx, glo->current_path);
	glo->current_path = NULL;
	close_doc(glo);
	fz_drop_context(glo->ctx);
	glo->ctx = NULL;
	free(glo);
#ifdef MEMENTO
	LOGI("Destroying dump start");
	Memento_listBlocks();
	Memento_stats();
	LOGI("Destroying dump end");
#endif
#ifdef NDK_PROFILER
	// Apparently we should really be writing to whatever path we get
	// from calling getFilesDir() in the java part, which supposedly
	// gives /sdcard/data/data/com.artifex.MuPDF/gmon.out, but that's
	// unfriendly.
	setenv("CPUPROFILE", "/sdcard/gmon.out", 1);
	moncleanup();
#endif
}

JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_getPageLinksInternal)(JNIEnv * env, jobject thiz, int pageNumber)
{
	jclass linkInfoClass;
	jclass linkInfoInternalClass;
	jclass linkInfoExternalClass;
	jclass linkInfoRemoteClass;
	jmethodID ctorInternal;
	jmethodID ctorExternal;
	jmethodID ctorRemote;
	jobjectArray arr;
	jobject linkInfo;
	fz_matrix ctm;
	float zoom;
	fz_link *list;
	fz_link *link;
	int count;
	page_cache *pc;
	globals *glo = get_globals(env, thiz);
    if (glo == NULL) return NULL;
	fz_context *ctx = glo->ctx;
	
    linkInfoClass = (*env)->FindClass(env, PACKAGENAME "/LinkInfo");
    if (linkInfoClass == NULL) fz_throw(glo->ctx, FZ_ERROR_GENERIC, "FindClass LinkInfo failed");
    linkInfoInternalClass = (*env)->FindClass(env, PACKAGENAME "/LinkInfoInternal");
    if (linkInfoInternalClass == NULL) fz_throw(glo->ctx, FZ_ERROR_GENERIC, "FindClass LinkInfoInternal failed");
    linkInfoExternalClass = (*env)->FindClass(env, PACKAGENAME "/LinkInfoExternal");
    if (linkInfoExternalClass == NULL) fz_throw(glo->ctx, FZ_ERROR_GENERIC, "FindClass LinkInfoExternal failed");
    linkInfoRemoteClass = (*env)->FindClass(env, PACKAGENAME "/LinkInfoRemote");
    if (linkInfoRemoteClass == NULL) fz_throw(glo->ctx, FZ_ERROR_GENERIC, "FindClass LinkInfoRemote failed");
    ctorInternal = (*env)->GetMethodID(env, linkInfoInternalClass, "<init>", "(FFFFIFFFFI)V");
    if (ctorInternal == NULL) fz_throw(glo->ctx, FZ_ERROR_GENERIC, "GetMethodID LinkInfoInternal() failed");
    ctorExternal = (*env)->GetMethodID(env, linkInfoExternalClass, "<init>", "(FFFFLjava/lang/String;)V");
    if (ctorExternal == NULL) fz_throw(glo->ctx, FZ_ERROR_GENERIC, "GetMethodID");
    ctorRemote = (*env)->GetMethodID(env, linkInfoRemoteClass, "<init>", "(FFFFLjava/lang/String;IZ)V");
    if (ctorRemote == NULL) fz_throw(glo->ctx, FZ_ERROR_GENERIC, "GetMethodID LinkInfoRemote() failed");

    JNI_FN(MuPDFCore_gotoPageInternal)(env, thiz, pageNumber);
    pc = &glo->pages[glo->current];
    if (pc->page == NULL || pc->number != pageNumber)
//        fz_throw(glo->ctx, FZ_ERROR_GENERIC, "MuPDFCore_gotoPageInternal failed");
        return NULL;

    zoom = glo->resolution / 72;
    fz_scale(&ctm, zoom, zoom);

    list = fz_load_links(ctx, pc->page);
    if (list == NULL)
//        fz_throw(glo->ctx, FZ_ERROR_GENERIC, "fz_load_links() returned NULL");
        return NULL;
    
    count = 0;
    for (link = list; link; link = link->next)
    {
        switch (link->dest.kind)
        {
            case FZ_LINK_GOTO:
            case FZ_LINK_GOTOR:
            case FZ_LINK_URI:
                count++ ;
        }
    }

    arr = (*env)->NewObjectArray(env, count, linkInfoClass, NULL);
    if (arr == NULL)
    {
        fz_drop_link(glo->ctx, list);
        fz_throw(glo->ctx, FZ_ERROR_GENERIC, "NewObjectArray() failed");
    }

    count = 0;
    for (link = list; link; link = link->next)
    {
        fz_rect rect = link->rect;
        fz_transform_rect(&rect, &ctm);

        switch (link->dest.kind)
        {
            case FZ_LINK_GOTO:
            {
                linkInfo = (*env)->NewObject(env, linkInfoInternalClass, ctorInternal,(float)rect.x0, (float)rect.y0, (float)rect.x1, (float)rect.y1,link->dest.ld.gotor.page,link->dest.ld.gotor.lt.x, link->dest.ld.gotor.lt.y , link->dest.ld.gotor.rb.x , link->dest.ld.gotor.rb.y, link->dest.ld.gotor.flags);
                break;
            }

            case FZ_LINK_GOTOR:
            {
                jstring juri = (*env)->NewStringUTF(env, link->dest.ld.gotor.file_spec);
                linkInfo = (*env)->NewObject(env, linkInfoRemoteClass, ctorRemote,
                                             (float)rect.x0, (float)rect.y0, (float)rect.x1, (float)rect.y1,
                                             juri, link->dest.ld.gotor.page, link->dest.ld.gotor.new_window ? JNI_TRUE : JNI_FALSE);
                break;
            }

            case FZ_LINK_URI:
            {
                jstring juri = (*env)->NewStringUTF(env, link->dest.ld.uri.uri);
                linkInfo = (*env)->NewObject(env, linkInfoExternalClass, ctorExternal,
                                             (float)rect.x0, (float)rect.y0, (float)rect.x1, (float)rect.y1,
                                             juri);
                break;
            }

            default:
                continue;
        }

        if (linkInfo == NULL)
        {
            fz_drop_link(glo->ctx, list);
            fz_throw(glo->ctx, FZ_ERROR_GENERIC, "linkInfo = NULL");
        }
        (*env)->SetObjectArrayElement(env, arr, count, linkInfo);
        (*env)->DeleteLocalRef(env, linkInfo);
        count++;
    }
    fz_drop_link(glo->ctx, list);

    return arr;
}

JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_getWidgetAreasInternal)(JNIEnv * env, jobject thiz, int pageNumber)
{
	jclass rectFClass;
	jmethodID ctor;
	jobjectArray arr;
	jobject rectF;
	pdf_document *idoc;
	pdf_widget *widget;
	fz_matrix ctm;
	float zoom;
	int count;
	page_cache *pc;
	globals *glo = get_globals(env, thiz);
	if (glo == NULL)
		return NULL;
	fz_context *ctx = glo->ctx;

	rectFClass = (*env)->FindClass(env, "android/graphics/RectF");
	if (rectFClass == NULL) return NULL;
	ctor = (*env)->GetMethodID(env, rectFClass, "<init>", "(FFFF)V");
	if (ctor == NULL) return NULL;

	JNI_FN(MuPDFCore_gotoPageInternal)(env, thiz, pageNumber);
	pc = &glo->pages[glo->current];
	if (pc->number != pageNumber || pc->page == NULL)
		return NULL;

	idoc = pdf_specifics(ctx, glo->doc);
	if (idoc == NULL)
		return NULL;

	zoom = glo->resolution / 72;
	fz_scale(&ctm, zoom, zoom);

	count = 0;
	for (widget = pdf_first_widget(ctx, idoc, (pdf_page *)pc->page); widget; widget = pdf_next_widget(ctx, widget))
		count ++;

	arr = (*env)->NewObjectArray(env, count, rectFClass, NULL);
	if (arr == NULL) return NULL;

	count = 0;
	for (widget = pdf_first_widget(ctx, idoc, (pdf_page *)pc->page); widget; widget = pdf_next_widget(ctx, widget))
	{
		fz_rect rect;
		pdf_bound_widget(ctx, widget, &rect);
		fz_transform_rect(&rect, &ctm);

		rectF = (*env)->NewObject(env, rectFClass, ctor,
				(float)rect.x0, (float)rect.y0, (float)rect.x1, (float)rect.y1);
		if (rectF == NULL) return NULL;
		(*env)->SetObjectArrayElement(env, arr, count, rectF);
		(*env)->DeleteLocalRef(env, rectF);

		count ++;
	}

	return arr;
}

JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_getAnnotationsInternal)(JNIEnv * env, jobject thiz, int pageNumber)
{
	jclass annotClass, pt_cls, ptarr_cls;
    jfieldID x_fid, y_fid;
    jmethodID Annotation;
    jmethodID PointF;
    jobjectArray arr;
    jobject jannot;
    fz_annot *annot;
    fz_matrix ctm;
    float zoom;
    int count;
    page_cache *pc;
    globals *glo = get_globals(env, thiz);
    fz_context *ctx = glo->ctx;
    
    if (glo == NULL) return NULL;

    annotClass = (*env)->FindClass(env, PACKAGENAME "/Annotation");
    if (annotClass == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "FindClass");
    
    Annotation = (*env)->GetMethodID(env, annotClass, "<init>", "(FFFFI[[Landroid/graphics/PointF;Ljava/lang/String;)V"); 
    if (Annotation == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "GetMethodID");

    pt_cls = (*env)->FindClass(env, "android/graphics/PointF");
    if (pt_cls == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "FindClass");
    x_fid = (*env)->GetFieldID(env, pt_cls, "x", "F");
    if (x_fid == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "GetFieldID(x)");
    y_fid = (*env)->GetFieldID(env, pt_cls, "y", "F");
    if (y_fid == NULL) fz_throw(ctx, FZ_ERROR_GENERIC, "GetFieldID(y)");
    PointF = (*env)->GetMethodID(env, pt_cls, "<init>", "(FF)V");
    
    JNI_FN(MuPDFCore_gotoPageInternal)(env, thiz, pageNumber);
    pc = &glo->pages[glo->current];
    if (pc->number != pageNumber || pc->page == NULL)
        return NULL;

    zoom = glo->resolution / 72;
    fz_scale(&ctm, zoom, zoom);

    count = 0;
    for (annot = fz_first_annot(ctx, pc->page); annot; annot = fz_next_annot(ctx, pc->page, annot))
        count ++;

    arr = (*env)->NewObjectArray(env, count, annotClass, NULL);
    if (arr == NULL) return NULL;

    count = 0;
    for (annot = fz_first_annot(ctx, pc->page); annot; annot = fz_next_annot(ctx, pc->page, annot))
    {
            //Get the type
        fz_annot_type type = pdf_annot_type(ctx, (pdf_annot *)annot);

            //Get the text of the annotatoin
        jstring jtext = NULL;
//        jchar * text16 = NULL;
        if(type == FZ_ANNOT_TEXT)
        {
//            pdf_obj * obj = pdf_annot_text((pdf_annot *)annot);
            unsigned short *text;
            unsigned int length;
            pdf_annot_text(ctx, (pdf_annot *)annot, &text, &length); //does the memory allocation!

            /* int i; */
            /* for (i=0; i< length; i++) */
            /*     LOGI("mupdf.c: shorts of annotation: %x", text[i]); */

//            LOGI("mupdf.c: first shorts of annotation: %x %x (%x %x)", text[0], text[1], 254, 255);
            
                //Try to find out the encoding of the string
            bool inHighBits = true;
            int i;
            for (i=0; i<length/2; i++)
            {
                if(text[i]<=255) {
                    inHighBits = false;
                    break;
                }
            }
            if(inHighBits && length >= 2) // => non standard encoding
            {
                    //LOGI("mupdf.c: assuming a non-standard encoding");
                length = length/2;
                jchar * text16 = (jchar *)(void *)text;
                int i;
                for (i=0; i< length; i++)
                    text16[i] = (text16[i]<<8) | (text16[i]>>8);
                jtext = (*env)->NewString(env, text16, length);
            }
            else if(text != NULL && length > 2 && ( (text[0] == 254 && text[1] == 255) || (text[0] == 255 && text[1] == 254) )) // => UTF-16
            {
                jchar * text16 = (jchar *)(void *)text;
                length = length/2;
                if (text16[0] == 0xfffe)
                { //We must swap the byte order because NewString() doesn't respect the BOM
                        //LOGI("mupdf.c: foud UTF-16LE encoding");
                    int i;
                    for (i=0; i< length; i++)
                        text16[i] = (text16[i]<<8) | (text16[i]>>8);
                }
                else
                {
                        //LOGI("mupdf.c: foud UTF-16BE encoding");
                }
                
                jtext = (*env)->NewString(env, text16, length);
            }
            else if(text != NULL && length > 0)
            {
                    //LOGI("mupdf.c: assuming PDFDocEncoding");
                
                /* int i; */
                /* for (i=0; i< length; i++) */
                /* LOGI("mupdf.c: shorts of annotation: %x", text[i]); */
                    //Check this properly!!!
                jchar *dstptr = (jchar *)malloc(length*sizeof(jchar));
                for (i = 0; i < length; i++)
                    if(text[i] <= 0x00ff)
                        dstptr[i] = PDFDocEncoding[text[i]];
                    else
                        dstptr[i] = text[i];
                jtext = (*env)->NewString(env, dstptr, length);
                free(dstptr);
            }

            free(text); //free here!
            
            /* int i; */
            /* for (i=0; i< length/2; i++) */
            /*     LOGI("mupdf.c: length=%d chars of annotation: %x", length, text[i]); */
            
            
//            unsigned int bufferLength = (pdf_to_str_len(obj) + 1)*2;
//            unsigned short *buffer = malloc(bufferLength);
//            pdf_to_ucs2_buf(buffer, obj);
//            jtext = (*env)->NewString(env, pdf_to_str_buf(obj), pdf_to_str_len(obj));
            /* jtext = (*env)->NewString(env, buffer, pdf_to_str_len(obj)); */
            /* free(buffer); */
            
            
/*             char * text = pdf_to_str_buf(obj); */
/*             int length = pdf_to_str_len(obj); */
/*                 //Test if the string is in UTF-16 or PDFDocEncoding */
/*             if(text != NULL && length > 2 && text[0] == 254 && text[1] == 255) */
/*             { */
/*                     //UTF-16 */
/* //                isUTF16 = true; */
/*                 text16 = (jchar *)(void *)text; */
/*                 length = length/2; */
/*                 if (text16[0] == 0xfffe)  */
/*                 { //So we must swap the byte order because NewString() doesn't respect the BOM */
/* //                    LOGI("mupdf.c: swapping the byte order because we found %x as first jchar", text16[0]); */
/*                     int i; */
/*                     for (i=0; i< length; i++) */
/*                         text16[i] = (text16[i]<<8) | (text16[i]>>8); */
/*                 } */

/*                 int i; */
/*                 for (i=0; i< length; i++) */
/*                 LOGI("mupdf.c: chars of annotation: %x", text16[i]); */
                
/*                 jtext = (*env)->NewString(env, text16, length); */
/*             } */
/*             else if(text != NULL && length > 0) */
/*             { */
/* //                isUTF16 = false; */
/*                     //PDFDocEncoding (some special characters can be lost here but nobobdy seems to use it anyway) */
/*                 jtext = (*env)->NewStringUTF(env, text); */
/*             } */
        }

        
            //Get the inklist
        jobjectArray arcs = NULL;
        if(type == FZ_ANNOT_INK)
        {
            pdf_obj *inklist = pdf_annot_inklist(ctx, (pdf_annot *)annot);
            int nArcs = pdf_array_len(ctx, inklist);
            int i;
            float pageHeight = (&glo->pages[glo->current])->height;
            for(i = 0; i < nArcs; i++)
            {
                pdf_obj *inklisti = pdf_array_get(ctx, inklist, i);
                int nArc = pdf_array_len(ctx, inklisti);
                jobjectArray arci = (*env)->NewObjectArray(env, nArc/2, pt_cls, NULL);
                
                if(i==0) { //Get the class of the array of pointF and create the array of arrays 
                    ptarr_cls = (*env)->GetObjectClass(env, arci);
                    if (ptarr_cls == NULL) {
                        fz_throw(glo->ctx, FZ_ERROR_GENERIC, "GetObjectClass()");
                    }
                    else {
                        arcs = (*env)->NewObjectArray(env, nArcs, ptarr_cls, NULL);
                        if (arcs == NULL) fz_throw(glo->ctx, FZ_ERROR_GENERIC, "arcs == NULL");
                    }
                }
                
                if (arci == NULL) return NULL;
                int j;
                for(j = 0; j < nArc; j+=2)
                {
                    fz_point point; 
                    point.x = pdf_to_real(ctx, pdf_array_get(ctx, inklisti, j));
                    point.y = pdf_to_real(ctx, pdf_array_get(ctx, inklisti, j+1));
                    fz_transform_point(&point, &ctm);
                    point.y = pageHeight - point.y;//Flip y here because pdf coordinate system is upside down
                    jobject pfobj = (*env)->NewObject(env, pt_cls, PointF, point.x, point.y);
                    (*env)->SetObjectArrayElement(env, arci, j/2, pfobj);
                    (*env)->DeleteLocalRef(env, pfobj);
            }
                (*env)->SetObjectArrayElement(env, arcs, i, arci);
                (*env)->DeleteLocalRef(env, arci);
            }
        }

            //Get the rect
        fz_rect rect;
        fz_bound_annot(ctx, glo->pages[glo->current].page, annot, &rect);
        fz_transform_rect(&rect, &ctm);

            //Creat the annotation
        if(Annotation != NULL)
        {
            jannot = (*env)->NewObject(env, annotClass, Annotation, (float)rect.x0, (float)rect.y0, (float)rect.x1, (float)rect.y1, type, arcs, jtext); 
        }
            
        if (jannot == NULL) return NULL;
        (*env)->SetObjectArrayElement(env, arr, count, jannot);

            //Clean up
        (*env)->DeleteLocalRef(env, jannot);
        (*env)->DeleteLocalRef(env, jtext);
        
        count ++;
    }

    return arr;
}

JNIEXPORT int JNICALL
JNI_FN(MuPDFCore_passClickEventInternal)(JNIEnv * env, jobject thiz, int pageNumber, float x, float y)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	fz_matrix ctm;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	float zoom;
	fz_point p;
	pdf_ui_event event;
	int changed = 0;
	page_cache *pc;

	if (idoc == NULL)
		return 0;

	JNI_FN(MuPDFCore_gotoPageInternal)(env, thiz, pageNumber);
	pc = &glo->pages[glo->current];
	if (pc->number != pageNumber || pc->page == NULL)
		return 0;

	p.x = x;
	p.y = y;

	/* Ultimately we should probably return a pointer to a java structure
	 * with the link details in, but for now, page number will suffice.
	 */
	zoom = glo->resolution / 72;
	fz_scale(&ctm, zoom, zoom);
	fz_invert_matrix(&ctm, &ctm);

	fz_transform_point(&p, &ctm);

	fz_try(ctx)
	{
		event.etype = PDF_EVENT_TYPE_POINTER;
		event.event.pointer.pt = p;
		event.event.pointer.ptype = PDF_POINTER_DOWN;
		changed = pdf_pass_event(ctx, idoc, (pdf_page *)pc->page, &event);
		event.event.pointer.ptype = PDF_POINTER_UP;
		changed |= pdf_pass_event(ctx, idoc, (pdf_page *)pc->page, &event);
		if (changed) {
			dump_annotation_display_lists(glo);
		}
	}
	fz_catch(ctx)
	{
		LOGE("passClickEvent: %s", ctx->error->message);
	}

	return changed;
}

JNIEXPORT jstring JNICALL
JNI_FN(MuPDFCore_getFocusedWidgetTextInternal)(JNIEnv * env, jobject thiz)
{
	char *text = "";
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;

	fz_try(ctx)
	{
		pdf_document *idoc = pdf_specifics(ctx, glo->doc);

		if (idoc)
		{
			pdf_widget *focus = pdf_focused_widget(ctx, idoc);

			if (focus)
				text = pdf_text_widget_text(ctx, idoc, focus);
		}
	}
	fz_catch(ctx)
	{
		LOGE("getFocusedWidgetText failed: %s", ctx->error->message);
	}

	return (*env)->NewStringUTF(env, text);
}

JNIEXPORT int JNICALL
JNI_FN(MuPDFCore_setFocusedWidgetTextInternal)(JNIEnv * env, jobject thiz, jstring jtext)
{
	const char *text;
	int result = 0;
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;

	text = (*env)->GetStringUTFChars(env, jtext, NULL);
	if (text == NULL)
	{
		LOGE("Failed to get text");
		return 0;
	}

	fz_try(ctx)
	{
		pdf_document *idoc = pdf_specifics(ctx, glo->doc);

		if (idoc)
		{
			pdf_widget *focus = pdf_focused_widget(ctx, idoc);

			if (focus)
			{
				result = pdf_text_widget_set_text(ctx, idoc, focus, (char *)text);
				dump_annotation_display_lists(glo);
			}
		}
	}
	fz_catch(ctx)
	{
		LOGE("setFocusedWidgetText failed: %s", ctx->error->message);
	}

	(*env)->ReleaseStringUTFChars(env, jtext, text);

	return result;
}

JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_getFocusedWidgetChoiceOptions)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	pdf_widget *focus;
	int type;
	int nopts, i;
	char **opts = NULL;
	jclass stringClass;
	jobjectArray arr;

	if (idoc == NULL)
		return NULL;

	focus = pdf_focused_widget(ctx, idoc);
	if (focus == NULL)
		return NULL;

	type = pdf_widget_get_type(ctx, focus);
	if (type != PDF_WIDGET_TYPE_LISTBOX && type != PDF_WIDGET_TYPE_COMBOBOX)
		return NULL;

	fz_var(opts);
	fz_try(ctx)
	{
		nopts = pdf_choice_widget_options(ctx, idoc, focus, 0, NULL);
		opts = fz_malloc(ctx, nopts * sizeof(*opts));
		(void)pdf_choice_widget_options(ctx, idoc, focus, 0, opts);
	}
	fz_catch(ctx)
	{
		fz_free(ctx, opts);
		LOGE("Failed in getFocuseedWidgetChoiceOptions");
		return NULL;
	}

	stringClass = (*env)->FindClass(env, "java/lang/String");

	arr = (*env)->NewObjectArray(env, nopts, stringClass, NULL);

	for (i = 0; i < nopts; i++)
	{
		jstring s = (*env)->NewStringUTF(env, opts[i]);
		if (s != NULL)
			(*env)->SetObjectArrayElement(env, arr, i, s);

		(*env)->DeleteLocalRef(env, s);
	}

	fz_free(ctx, opts);

	return arr;
}

JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_getFocusedWidgetChoiceSelected)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	pdf_widget *focus;
	int type;
	int nsel, i;
	char **sel = NULL;
	jclass stringClass;
	jobjectArray arr;

	if (idoc == NULL)
		return NULL;

	focus = pdf_focused_widget(ctx, idoc);
	if (focus == NULL)
		return NULL;

	type = pdf_widget_get_type(ctx, focus);
	if (type != PDF_WIDGET_TYPE_LISTBOX && type != PDF_WIDGET_TYPE_COMBOBOX)
		return NULL;

	fz_var(sel);
	fz_try(ctx)
	{
		nsel = pdf_choice_widget_value(ctx, idoc, focus, NULL);
		sel = fz_malloc(ctx, nsel * sizeof(*sel));
		(void)pdf_choice_widget_value(ctx, idoc, focus, sel);
	}
	fz_catch(ctx)
	{
		fz_free(ctx, sel);
		LOGE("Failed in getFocuseedWidgetChoiceOptions");
		return NULL;
	}

	stringClass = (*env)->FindClass(env, "java/lang/String");

	arr = (*env)->NewObjectArray(env, nsel, stringClass, NULL);

	for (i = 0; i < nsel; i++)
	{
		jstring s = (*env)->NewStringUTF(env, sel[i]);
		if (s != NULL)
			(*env)->SetObjectArrayElement(env, arr, i, s);

		(*env)->DeleteLocalRef(env, s);
	}

	fz_free(ctx, sel);

	return arr;
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_setFocusedWidgetChoiceSelectedInternal)(JNIEnv * env, jobject thiz, jobjectArray arr)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	pdf_widget *focus;
	int type;
	int nsel, i;
	char **sel = NULL;
	jstring *objs = NULL;

	if (idoc == NULL)
		return;

	focus = pdf_focused_widget(ctx, idoc);
	if (focus == NULL)
		return;

	type = pdf_widget_get_type(ctx, focus);
	if (type != PDF_WIDGET_TYPE_LISTBOX && type != PDF_WIDGET_TYPE_COMBOBOX)
		return;

	nsel = (*env)->GetArrayLength(env, arr);

	sel = calloc(nsel, sizeof(*sel));
	objs = calloc(nsel, sizeof(*objs));
	if (objs == NULL || sel == NULL)
	{
		free(sel);
		free(objs);
		LOGE("Failed in setFocusWidgetChoiceSelected");
		return;
	}

	for (i = 0; i < nsel; i++)
	{
		objs[i] = (jstring)(*env)->GetObjectArrayElement(env, arr, i);
		sel[i] = (char *)(*env)->GetStringUTFChars(env, objs[i], NULL);
	}

	fz_try(ctx)
	{
		pdf_choice_widget_set_value(ctx, idoc, focus, nsel, sel);
		dump_annotation_display_lists(glo);
	}
	fz_catch(ctx)
	{
		LOGE("Failed in setFocusWidgetChoiceSelected");
	}

	for (i = 0; i < nsel; i++)
		(*env)->ReleaseStringUTFChars(env, objs[i], sel[i]);

	free(sel);
	free(objs);
}

JNIEXPORT int JNICALL
JNI_FN(MuPDFCore_getFocusedWidgetTypeInternal)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	pdf_widget *focus;

	if (ctx, idoc == NULL)
		return NONE;

	focus = pdf_focused_widget(ctx, idoc);

	if (focus == NULL)
		return NONE;

	switch (pdf_widget_get_type(ctx, focus))
	{
	case PDF_WIDGET_TYPE_TEXT: return TEXT;
	case PDF_WIDGET_TYPE_LISTBOX: return LISTBOX;
	case PDF_WIDGET_TYPE_COMBOBOX: return COMBOBOX;
	case PDF_WIDGET_TYPE_SIGNATURE: return SIGNATURE;
	}

	return NONE;
}

/* This enum should be kept in line with SignatureState in MuPDFPageView.java */
enum
{
	Signature_NoSupport,
	Signature_Unsigned,
	Signature_Signed
};

JNIEXPORT int JNICALL
JNI_FN(MuPDFCore_getFocusedWidgetSignatureState)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	pdf_widget *focus;

	if (ctx, idoc == NULL)
		return Signature_NoSupport;

	focus = pdf_focused_widget(ctx, idoc);

	if (focus == NULL)
		return Signature_NoSupport;

	if (!pdf_signatures_supported())
		return Signature_NoSupport;

	return pdf_dict_get(ctx, ((pdf_annot *)focus)->obj, PDF_NAME_V) ? Signature_Signed : Signature_Unsigned;
}

JNIEXPORT jstring JNICALL
JNI_FN(MuPDFCore_checkFocusedSignatureInternal)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	pdf_widget *focus;
	char ebuf[256] = "Failed";

	if (idoc == NULL)
		goto exit;

	focus = pdf_focused_widget(ctx, idoc);

	if (focus == NULL)
		goto exit;

	if (pdf_check_signature(ctx, idoc, focus, glo->current_path, ebuf, sizeof(ebuf)))
	{
		strcpy(ebuf, "Signature is valid");
	}

exit:
	return (*env)->NewStringUTF(env, ebuf);
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_signFocusedSignatureInternal)(JNIEnv * env, jobject thiz, jstring jkeyfile, jstring jpassword)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);
	pdf_widget *focus;
	const char *keyfile;
	const char *password;
	jboolean res;

	if (idoc == NULL)
		return JNI_FALSE;

	focus = pdf_focused_widget(ctx, idoc);

	if (focus == NULL)
		return JNI_FALSE;

	keyfile = (*env)->GetStringUTFChars(env, jkeyfile, NULL);
	password = (*env)->GetStringUTFChars(env, jpassword, NULL);
	if (keyfile == NULL || password == NULL)
		return JNI_FALSE;

	fz_var(res);
	fz_try(ctx)
	{
		pdf_sign_signature(ctx, idoc, focus, keyfile, password);
		dump_annotation_display_lists(glo);
		res = JNI_TRUE;
	}
	fz_catch(ctx)
	{
		res = JNI_FALSE;
	}

	return res;
}

JNIEXPORT jobject JNICALL
JNI_FN(MuPDFCore_waitForAlertInternal)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	jclass alertClass;
	jmethodID ctor;
	jstring title;
	jstring message;
	int alert_present;
	pdf_alert_event alert;

	LOGT("Enter waitForAlert");
	pthread_mutex_lock(&glo->fin_lock);
	pthread_mutex_lock(&glo->alert_lock);

	while (glo->alerts_active && !glo->alert_request)
		pthread_cond_wait(&glo->alert_request_cond, &glo->alert_lock);
	glo->alert_request = 0;

	alert_present = (glo->alerts_active && glo->current_alert);

	if (alert_present)
		alert = *glo->current_alert;

	pthread_mutex_unlock(&glo->alert_lock);
	pthread_mutex_unlock(&glo->fin_lock);
	LOGT("Exit waitForAlert %d", alert_present);

	if (!alert_present)
		return NULL;

	alertClass = (*env)->FindClass(env, PACKAGENAME "/MuPDFAlertInternal");
	if (alertClass == NULL)
		return NULL;

	ctor = (*env)->GetMethodID(env, alertClass, "<init>", "(Ljava/lang/String;IILjava/lang/String;I)V");
	if (ctor == NULL)
		return NULL;

	title = (*env)->NewStringUTF(env, alert.title);
	if (title == NULL)
		return NULL;

	message = (*env)->NewStringUTF(env, alert.message);
	if (message == NULL)
		return NULL;

	return (*env)->NewObject(env, alertClass, ctor, message, alert.icon_type, alert.button_group_type, title, alert.button_pressed);
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_replyToAlertInternal)(JNIEnv * env, jobject thiz, jobject alert)
{
	globals *glo = get_globals(env, thiz);
	jclass alertClass;
	jfieldID field;
	int button_pressed;

	alertClass = (*env)->FindClass(env, PACKAGENAME "/MuPDFAlertInternal");
	if (alertClass == NULL)
		return;

	field = (*env)->GetFieldID(env, alertClass, "buttonPressed", "I");
	if (field == NULL)
		return;

	button_pressed = (*env)->GetIntField(env, alert, field);

	LOGT("Enter replyToAlert");
	pthread_mutex_lock(&glo->alert_lock);

	if (glo->alerts_active && glo->current_alert)
	{
		// Fill in button_pressed and signal reply received.
		glo->current_alert->button_pressed = button_pressed;
		glo->alert_reply = 1;
		pthread_cond_signal(&glo->alert_reply_cond);
	}

	pthread_mutex_unlock(&glo->alert_lock);
	LOGT("Exit replyToAlert");
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_startAlertsInternal)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);

	if (!glo->alerts_initialised)
		return;

	LOGT("Enter startAlerts");
	pthread_mutex_lock(&glo->alert_lock);

	glo->alert_reply = 0;
	glo->alert_request = 0;
	glo->alerts_active = 1;
	glo->current_alert = NULL;

	pthread_mutex_unlock(&glo->alert_lock);
	LOGT("Exit startAlerts");
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_stopAlertsInternal)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);

	if (!glo->alerts_initialised)
		return;

	LOGT("Enter stopAlerts");
	pthread_mutex_lock(&glo->alert_lock);

	glo->alert_reply = 0;
	glo->alert_request = 0;
	glo->alerts_active = 0;
	glo->current_alert = NULL;
	pthread_cond_signal(&glo->alert_reply_cond);
	pthread_cond_signal(&glo->alert_request_cond);

	pthread_mutex_unlock(&glo->alert_lock);
	LOGT("Exit stopAleerts");
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_hasChangesInternal)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	pdf_document *idoc = pdf_specifics(ctx, glo->doc);

	return (idoc && pdf_has_unsaved_changes(ctx, idoc)) ? JNI_TRUE : JNI_FALSE;
}

static const char * android_tmp_folder(JNIEnv * env) {
    jclass coreClass = (*env)->FindClass(env, PACKAGENAME "/MuPDFCore");
   jmethodID getCacheDir = (*env)->GetStaticMethodID(env, coreClass, "getCacheDir", "()Ljava/lang/String;");
   jstring cache_dir = (jstring)(*env)->CallStaticObjectMethod(env, coreClass, getCacheDir);
//	jstring cache_dir = (*env)->GetStaticFieldID(env, coreClass, "cachDir", "Ljava/lang/String");
    const char *path_chars = (*env)->GetStringUTFChars(env, cache_dir, NULL);
    return path_chars;
}

static char *tmp_path(JNIEnv * env, char *path)
{
	int rnd_length = 6;
	char *rnd = malloc(sizeof(char) * (rnd_length +1));
	if (!rnd)
		return NULL;
	int i;
	for (i=0; i<rnd_length; i++)
		rnd[i] = "0123456789abcdef"[random() % 16];
	rnd[rnd_length] = '\0';
	
	int f;
	char *buf = malloc(strlen(path) + 1 + strlen(rnd) + 4 + 1);
	if (!buf)
		return NULL;
	
	strcpy(buf, path);
	strcat(buf, "_");
	strcat(buf, rnd);
	strcat(buf, ".pdf");

	return buf;
	
	/* f = mkstemp(buf); //mkstemp() is broke on android and rename() (which we use below) can fail if we try to rename from the official cach directory, so we have no choice but to generate a tmp path by hand in the given directoy...*/
	
	/* if (f >= 0) */
	/* { */
	/* 	close(f); */
	/* 	return buf; */
	/* } */
	/* else */
	/* { */
	/* 	free(buf); */
	/* 	return NULL; */
	/* } */
}

JNIEXPORT int JNICALL
JNI_FN(MuPDFCore_saveAsInternal)(JNIEnv *env, jobject thiz, jstring jpath)
{    
    globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;

        //Try to get the new path from jpath
        //If jpath was null we leave new_path = NULL
    const char *new_path = NULL;
    if (jpath != NULL) 
    {
        new_path = (*env)->GetStringUTFChars(env, jpath, NULL);
    }
    LOGI("Core: current_path=%s new_path=%s", glo->current_path, new_path);
	
	int written = 0;
    if (glo->doc != NULL && (glo->current_path != NULL || new_path != NULL) )
    {
        char *tmp;
        fz_write_options opts;

            //Generate temporary path in an appropriate location
        if (new_path != NULL)
            tmp = tmp_path(env, (char *)new_path);
        else
            tmp = tmp_path(env, glo->current_path);
        LOGI("Core: tmp=%s", tmp);
        if (tmp)
        {
            
                //We save incremental if the current path is set first copying from current path
			fz_var(written);
			fz_try(ctx)
			{
				if (glo->current_path)
				{
					opts.do_incremental = 1;
					opts.do_ascii = 0;
					opts.do_expand = 0;
					opts.do_garbage = 0;
					opts.do_linear = 0;
					opts.do_clean = 0; // enabling cleaning leads to a heap corruption during a fz_malloc in pdf_clean_page_contents
					
                    FILE *fin = fopen(glo->current_path, "rb");
                    FILE *fout = fopen(tmp, "wb");
                    char buf[256];
                    int n, err = 1;
                    
                    if (fin && fout)
                    {
                        while ((n = fread(buf, 1, sizeof(buf), fin)) > 0)
                            fwrite(buf, 1, n, fout);
                        err = (ferror(fin) || ferror(fout));
                    }
                    
                    if (fin)
                        fclose(fin);
                    if (fout)
                        fclose(fout);
                    
                    if (!err)
                    {
                        fz_write_document(ctx, glo->doc, tmp, &opts);
                        written = 1;
                    }
				}
				else
				{
					opts.do_incremental = 0;
					opts.do_ascii = 1;
					opts.do_expand = 0;
					opts.do_garbage = 1;
					opts.do_linear = 0;
					opts.do_clean = 0; // enabling cleaning leads to a heap corruption during a fz_malloc in pdf_clean_page_contents
										
					FILE *fout = fopen(tmp, "wb");
					if (fout)
					{
						fclose(fout);
						fz_write_document(ctx, glo->doc, tmp, &opts);
						written = 1;
					}
				}
			}
			fz_catch(ctx)
			{
				written = 0;
			}
			
            if (written)
            {
                LOGI("Core: closing");
                close_doc(glo);
				int rename_st;
                if (new_path == NULL)
                    rename_st = rename(tmp, glo->current_path);
                else
                    rename_st = rename(tmp, new_path);
                if(rename_st != 0)
					written = 0;
            }
            free(tmp);
        }
    }
    if (jpath != NULL && new_path != NULL) (*env)->ReleaseStringUTFChars(env, jpath, new_path);
	
    return written-1; //return -1 on error and 0 on success 
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_saveInternal)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;

	if (glo->doc && glo->current_path)
	{
		char *tmp;
		fz_write_options opts;
		opts.do_incremental = 1;
		opts.do_ascii = 0;
		opts.do_expand = 0;
		opts.do_garbage = 0;
		opts.do_linear = 0;

		tmp = tmp_path(env, glo->current_path);
		if (tmp)
		{
			int written = 0;

			fz_var(written);
			fz_try(ctx)
			{
				FILE *fin = fopen(glo->current_path, "rb");
				FILE *fout = fopen(tmp, "wb");
				char buf[256];
				int n, err = 1;

				if (fin && fout)
				{
					while ((n = fread(buf, 1, sizeof(buf), fin)) > 0)
						fwrite(buf, 1, n, fout);
					err = (ferror(fin) || ferror(fout));
				}

				if (fin)
					fclose(fin);
				if (fout)
					fclose(fout);

				if (!err)
				{
					fz_write_document(ctx, glo->doc, tmp, &opts);
					written = 1;
				}
			}
			fz_catch(ctx)
			{
				written = 0;
			}

			if (written)
			{
				close_doc(glo);
				rename(tmp, glo->current_path);
			}

			free(tmp);
		}
	}
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_dumpMemoryInternal)(JNIEnv * env, jobject thiz)
{
    globals *glo = get_globals(env, thiz);
    if (glo == NULL) return;
    fz_context *ctx = glo->ctx;

#ifdef MEMENTO
	LOGE("dumpMemoryInternal start");
	Memento_listNewBlocks();
	Memento_stats();
	LOGE("dumpMemoryInternal end");
#endif
}

JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_setInkThickness)(JNIEnv * env, jobject thiz, float inkThickness)
{
    globals *glo = get_globals(env, thiz);
    if (glo == NULL) return NULL;
    glo->inkThickness = inkThickness;
}


JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_setInkColor)(JNIEnv * env, jobject thiz, float r, float g, float b)
{
    globals *glo = get_globals(env, thiz);
    if (glo == NULL) return NULL;
    glo->inkColor[0] = r;
    glo->inkColor[1] = g;
    glo->inkColor[2] = b;
}


JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_setHighlightColor)(JNIEnv * env, jobject thiz, float r, float g, float b)
{
    globals *glo = get_globals(env, thiz);
    if (glo == NULL) return NULL;
    glo->highlightColor[0] = r;
    glo->highlightColor[1] = g;
    glo->highlightColor[2] = b;
}


JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_setUnderlineColor)(JNIEnv * env, jobject thiz, float r, float g, float b)
{
    globals *glo = get_globals(env, thiz);
    if (glo == NULL) return NULL;
    glo->underlineColor[0] = r;
    glo->underlineColor[1] = g;
    glo->underlineColor[2] = b;
}


JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_setStrikeoutColor)(JNIEnv * env, jobject thiz, float r, float g, float b)
{
    globals *glo = get_globals(env, thiz);
    if (glo == NULL) return NULL;
    glo->strikeoutColor[0] = r;
    glo->strikeoutColor[1] = g;
    glo->strikeoutColor[2] = b;
}

JNIEXPORT jobjectArray JNICALL
JNI_FN(MuPDFCore_setTextAnnotIconColor)(JNIEnv * env, jobject thiz, float r, float g, float b)
{
    globals *glo = get_globals(env, thiz);
    if (glo == NULL) return NULL;
    glo->textAnnotIconColor[0] = r;
    glo->textAnnotIconColor[1] = g;
    glo->textAnnotIconColor[2] = b;
}

JNIEXPORT int JNICALL
JNI_FN(MuPDFCore_insertBlankPageBeforeInternal)(JNIEnv * env, jobject thiz, int position)
{
    globals *glo = get_globals(env, thiz);
    if (glo == NULL) return 0;
    fz_document *doc = glo->doc;
    fz_context *ctx = glo->ctx;
    page_cache *pc = &glo->pages[glo->current];
    pdf_page * page = pdf_create_page(ctx, (pdf_document *)doc, pc->media_box, 72, 0);
    pdf_insert_page(ctx, (pdf_document *)doc, page, position);
    pdf_finish_edit(ctx, (pdf_document *)doc);
    pdf_drop_page(ctx, page);
    return 0;
}

JNIEXPORT jlong JNICALL
JNI_FN(MuPDFCore_createCookie)(JNIEnv * env, jobject thiz)
{
	globals *glo = get_globals_any_thread(env, thiz);
	if (glo == NULL)
		return 0;
	fz_context *ctx = glo->ctx;

	return (jlong) (intptr_t) fz_calloc_no_throw(ctx,1, sizeof(fz_cookie));
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_destroyCookie)(JNIEnv * env, jobject thiz, jlong cookiePtr)
{
	fz_cookie *cookie = (fz_cookie *) (intptr_t) cookiePtr;
	globals *glo = get_globals_any_thread(env, thiz);
	if (glo == NULL)
		return;
	fz_context *ctx = glo->ctx;
	if (ctx == NULL)
		return;
	
	fz_free(ctx, cookie);
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_abortCookie)(JNIEnv * env, jobject thiz, jlong cookiePtr)
{
	fz_cookie *cookie = (fz_cookie *) (intptr_t) cookiePtr;
	if (cookie != NULL)
		cookie->abort = 1;
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_cookieAborted)(JNIEnv * env, jobject thiz, jlong cookiePtr)
{
	fz_cookie *cookie = (fz_cookie *) (intptr_t) cookiePtr;
	if (cookie == NULL || cookie->abort == 1)
		return true;
    else
        return false;
}


static char *tmp_gproof_path(char *path)
{
	FILE *f;
	int i;
	char *buf = malloc(strlen(path) + 20 + 1);
	if (!buf)
		return NULL;

	for (i = 0; i < 10000; i++)
	{
		sprintf(buf, "%s.%d.gproof", path, i);

		LOGI("Trying for %s\n", buf);
		f = fopen(buf, "r");
		if (f != NULL)
		{
			fclose(f);
			continue;
		}

		f = fopen(buf, "w");
		if (f != NULL)
		{
			fclose(f);
			break;
		}
	}
	if (i == 10000)
	{
		LOGE("Failed to find temp gproof name");
		free(buf);
		return NULL;
	}

	LOGI("Rewritten to %s\n", buf);
	return buf;
}

JNIEXPORT jstring JNICALL
JNI_FN(MuPDFCore_startProofInternal)(JNIEnv * env, jobject thiz, int inResolution)
{
#ifdef SUPPORT_GPROOF
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	char *tmp;
	jstring ret;

	if (!glo->doc || !glo->current_path)
		return NULL;

	tmp = tmp_gproof_path(glo->current_path);
	if (!tmp)
		return NULL;

	int theResolution = PROOF_RESOLUTION;
	if (inResolution != 0)
		theResolution = inResolution;

	fz_try(ctx)
	{
		fz_write_gproof_file(ctx, glo->current_path, glo->doc, tmp, theResolution, "", "");

		LOGI("Creating %s\n", tmp);
		ret = (*env)->NewStringUTF(env, tmp);
	}
	fz_always(ctx)
	{
		free(tmp);
	}
	fz_catch(ctx)
	{
		ret = NULL;
	}
	return ret;
#else
	return NULL;
#endif
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_endProofInternal)(JNIEnv * env, jobject thiz, jstring jfilename)
{
#ifdef SUPPORT_GPROOF
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	const char *tmp;

	if (!glo->doc || !glo->current_path || jfilename == NULL)
		return;

	tmp = (*env)->GetStringUTFChars(env, jfilename, NULL);
	if (tmp)
	{
		LOGI("Deleting %s\n", tmp);

		unlink(tmp);
		(*env)->ReleaseStringUTFChars(env, jfilename, tmp);
	}
#endif
}

JNIEXPORT jboolean JNICALL
JNI_FN(MuPDFCore_gprfSupportedInternal)(JNIEnv * env)
{
#ifdef SUPPORT_GPROOF
	return JNI_TRUE;
#else
	return JNI_FALSE;
#endif
}

JNIEXPORT int JNICALL
JNI_FN(MuPDFCore_getNumSepsOnPageInternal)(JNIEnv *env, jobject thiz, int page)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	int i;

	for (i = 0; i < NUM_CACHE; i++)
	{
		if (glo->pages[i].page != NULL && glo->pages[i].number == page)
			break;
	}
	if (i == NUM_CACHE)
		return 0;

	LOGI("Counting seps on page %d", page);

	return fz_count_separations_on_page(ctx, glo->pages[i].page);
}

JNIEXPORT void JNICALL
JNI_FN(MuPDFCore_controlSepOnPageInternal)(JNIEnv *env, jobject thiz, int page, int sep, jboolean disable)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	int i;

	for (i = 0; i < NUM_CACHE; i++)
	{
		if (glo->pages[i].page != NULL && glo->pages[i].number == page)
			break;
	}
	if (i == NUM_CACHE)
		return;

	fz_control_separation_on_page(ctx, glo->pages[i].page, sep, disable);
}

JNIEXPORT jobject JNICALL
JNI_FN(MuPDFCore_getSepInternal)(JNIEnv *env, jobject thiz, int page, int sep)
{
	globals *glo = get_globals(env, thiz);
	fz_context *ctx = glo->ctx;
	const char *name;
	char rgba[4];
	unsigned int bgra;
	unsigned int cmyk;
	jobject jname;
	jclass sepClass;
	jmethodID ctor;
	int i;

	for (i = 0; i < NUM_CACHE; i++)
	{
		if (glo->pages[i].page != NULL && glo->pages[i].number == page)
			break;
	}
	if (i == NUM_CACHE)
		return NULL;

	/* MuPDF returns RGBA as bytes. Android wants a packed BGRA int. */
	name = fz_get_separation_on_page(ctx, glo->pages[i].page, sep, (unsigned int *)(&rgba[0]), &cmyk);
	bgra = (rgba[0] << 16) | (rgba[1]<<8) | rgba[2] | (rgba[3]<<24);
	jname = name ? (*env)->NewStringUTF(env, name) : NULL;

	sepClass = (*env)->FindClass(env, PACKAGENAME "/Separation");
	if (sepClass == NULL)
		return NULL;

	ctor = (*env)->GetMethodID(env, sepClass, "<init>", "(Ljava/lang/String;II)V");
	if (ctor == NULL)
		return NULL;

	return (*env)->NewObject(env, sepClass, ctor, jname, bgra, cmyk);
}
