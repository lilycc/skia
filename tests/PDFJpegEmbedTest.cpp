/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkDocument.h"
#include "SkCanvas.h"
#include "SkImageGenerator.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkDecodingImageGenerator.h"

#include "Resources.h"
#include "Test.h"

// Returned bitmap is lazy.  Only lazy bitmaps hold onto the original data.
static SkBitmap bitmap_from_data(SkData* data) {
    SkASSERT(data);
    SkBitmap bm;
    SkInstallDiscardablePixelRef(
            SkDecodingImageGenerator::Create(
                    data, SkDecodingImageGenerator::Options()), &bm);
    return bm;
}

static bool is_subset_of(SkData* smaller, SkData* larger) {
    SkASSERT(smaller && larger);
    if (smaller->size() > larger->size()) {
        return false;
    }
    size_t size = smaller->size();
    size_t size_diff = larger->size() - size;
    for (size_t i = 0; i <= size_diff; ++i) {
        if (0 == memcmp(larger->bytes() + i, smaller->bytes(), size)) {
            return true;
        }
    }
    return false;
}


static SkData* load_resource(
        skiatest::Reporter* r, const char* test, const char* filename) {
    SkString path(GetResourcePath(filename));
    SkData* data = SkData::NewFromFileName(path.c_str());
    if (!data && r->verbose()) {
        SkDebugf("\n%s: Resource '%s' can not be found.\n",
                 test, filename);
    }
    return data;  // May return NULL.
}

/**
 *  Test that for Jpeg files that use the JFIF colorspace, they are
 *  directly embedded into the PDF (without re-encoding) when that
 *  makes sense.
 */
DEF_TEST(PDFJpegEmbedTest, r) {
    const char test[] = "PDFJpegEmbedTest";
    SkAutoTUnref<SkData> mandrillData(
            load_resource(r, test, "mandrill_512_q075.jpg"));
    SkAutoTUnref<SkData> cmykData(load_resource(r, test, "CMYK.jpg"));
    if (!mandrillData || !cmykData) {
        return;
    }

    SkDynamicMemoryWStream pdf;
    SkAutoTUnref<SkDocument> document(SkDocument::CreatePDF(&pdf));
    SkCanvas* canvas = document->beginPage(642, 1028);

    canvas->clear(SK_ColorLTGRAY);

    SkBitmap bm1(bitmap_from_data(mandrillData));
    canvas->drawBitmap(bm1, 65.0, 0.0, NULL);
    SkBitmap bm2(bitmap_from_data(cmykData));
    canvas->drawBitmap(bm2, 0.0, 512.0, NULL);

    canvas->flush();
    document->endPage();
    document->close();
    SkAutoTUnref<SkData> pdfData(pdf.copyToData());
    SkASSERT(pdfData);
    pdf.reset();

    // Test disabled, waiting on resolution to http://skbug.com/3180
    // REPORTER_ASSERT(r, is_subset_of(mandrillData, pdfData));

    // This JPEG uses a nonstandard colorspace - it can not be
    // embedded into the PDF directly.
    REPORTER_ASSERT(r, !is_subset_of(cmykData, pdfData));

    // The following is for debugging purposes only.
    const char* outputPath = getenv("SKIA_TESTS_PDF_JPEG_EMBED_OUTPUT_PATH");
    if (outputPath) {
        SkFILEWStream output(outputPath);
        if (output.isValid()) {
            output.write(pdfData->data(), pdfData->size());
        }
    }
}
