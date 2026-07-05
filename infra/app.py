#!/usr/bin/env python3
import aws_cdk as cdk

from lib.wastewatch_ocr_stack import WasteWatchOcrStack

app = cdk.App()
WasteWatchOcrStack(app, "WasteWatchOcrStack")
app.synth()
