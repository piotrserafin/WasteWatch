from aws_cdk import (
    Stack,
    Duration,
    CfnOutput,
    aws_lambda as lambda_,
    aws_iam as iam,
)
from constructs import Construct


class WasteWatchOcrStack(Stack):

    def __init__(self, scope: Construct, id: str, **kwargs) -> None:
        super().__init__(scope, id, **kwargs)

        stack = Stack.of(self)

        lambda_role = iam.Role(
            self,
            f"{stack.stack_name}-lambda-execution-role",
            assumed_by=iam.ServicePrincipal("lambda.amazonaws.com"),
            role_name=f"{stack.stack_name}-lambda-execution-role",
            managed_policies=[
                iam.ManagedPolicy.from_managed_policy_arn(
                    self,
                    "cloudwatch-full-access",
                    "arn:aws:iam::aws:policy/CloudWatchFullAccess",
                ),
            ],
        )

        # https://github.com/bweigel/aws-lambda-tesseract-layer
        tesseract_layer = lambda_.LayerVersion(
            self,
            "TesseractLayer",
            code=lambda_.Code.from_asset("layer/tesseract-al2023-x86.zip"),
            compatible_runtimes=[lambda_.Runtime.PYTHON_3_14],
            description="Tesseract 5.4 OCR for AL2023",
        )

        ocr_function = lambda_.Function(
            self,
            f"{stack.stack_name}-lambda",
            description="OCR proxy for kiedywywoz.pl waste collection schedule",
            code=lambda_.Code.from_asset("lambda"),
            runtime=lambda_.Runtime.PYTHON_3_14,
            handler="handler.lambda_handler",
            layers=[tesseract_layer],
            memory_size=512,
            timeout=Duration.seconds(60),
            role=lambda_role,
            function_name=f"{stack.stack_name}-lambda",
            environment={
                "TESSDATA_PREFIX": "/opt/tesseract/share/tessdata",
                "LD_LIBRARY_PATH": "/opt/lib:/var/lang/lib:/lib64:/usr/lib64",
                "PATH": "/opt/bin:/var/lang/bin:/usr/local/bin:/usr/bin:/bin",
            },
        )

        function_url = ocr_function.add_function_url(
            auth_type=lambda_.FunctionUrlAuthType.NONE,
            cors=lambda_.FunctionUrlCorsOptions(
                allowed_methods=[lambda_.HttpMethod.ALL],
                allowed_headers=["*"],
                allowed_origins=["*"],
            ),
        )

        CfnOutput(
            self,
            "WasteWatchOcrFunctionURL",
            value=function_url.url,
            description="WasteWatch OCR Lambda Function URL",
        )
