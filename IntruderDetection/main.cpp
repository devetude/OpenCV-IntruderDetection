#include "opencv/cv.h"
#include "opencv/highgui.h"

#include <Windows.h>
#include <iostream>

#define WIN_NAME "침입자 탐지"
#define DIFF_THRESHOLD 0.1
#define FACE_CLASSIFIER_PATH "C:\\Program Files\\opencv\\sources\\data\\haarcascades\\haarcascade_frontalface_default.xml"
#define FACE_SEARCH_SCALE 1.1
#define MERGE_DETECTED_GROUP_CNTS 3
#define FACE_FRAME_WIDTH 50
#define FACE_FRAME_HEIGHT 50
#define FACE_FRAME_THICKNESS 1
#define INTRUDER_IMAGES_SAVE_PATH "..\\intruder_images\\"

/**
*	현재 시간정보를 문자열로 가져오는 메소드
*/
const std::string getCurrentTS2Str() {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", &tstruct);

	return buf;
}

/**
*	메인 메소드
*/
int main(int argc, char *argv[]) {
	// 웹 캠 생성
	cv::VideoCapture capture(0);

	// 웹 캠을 실행하지 못한 경우 에러 출력 및 종료
	if (!capture.isOpened()) {
		std::cerr << "웹 캠 디바이스를 찾을 수 없습니다." << std::endl;
		return 0;
	}

	// 윈도우 생성
	cv::namedWindow(WIN_NAME, 1);

	// 얼굴인식 템플릿 설정
	cv::CascadeClassifier face_classifier;
	face_classifier.load(FACE_CLASSIFIER_PATH);

	std::cout << "'esc' 키를 이용하여 프로그램 종료 " << std::endl;

	// 이전 그레이 스케일 프레임을 저장하는 변수
	cv::Mat frameBeforeGrayScale;

	while (true) {
		bool isFrameValid = true;
		cv::Mat frameOriginalMat;
		cv::Mat frame;

		try {
			// 웹 캠 프레임의 원본 크기 저장
			capture >> frameOriginalMat;

			// 원본 크기의 1/2로 축소 (프레임의 크기가 클 경우 연산시간이 증가)
			cv::resize(frameOriginalMat, frame, cv::Size(frameOriginalMat.cols / 2, frameOriginalMat.rows / 2), 0, 0, CV_INTER_NN);
		} catch (cv::Exception& e) {
			// 에러 출력
			std::cerr << "프레임 축소에 실패했기에, 해당 프레임을 무시합니다." << e.err << std::endl;
			isFrameValid = false;
		}

		// 현재 프레임을 그레이 스케일로 변경
		cv::Mat frameCurrentGrayScale;
		cv::cvtColor(frame, frameCurrentGrayScale, CV_BGR2GRAY);

		if (frameCurrentGrayScale.size == frameBeforeGrayScale.size) {
			// 정규화를 통해 이전 프레임과 현재 프레임의 차이의 정도를 구함
			double errorL2 = cv::norm(frameCurrentGrayScale, frameBeforeGrayScale, CV_L2);
			double diff = errorL2 / (double)(frameCurrentGrayScale.rows * frameBeforeGrayScale.rows);

			// 임계치 값보다 프레임의 차이가 클 경우
			if (diff >= DIFF_THRESHOLD) {
				// 얼굴영역을 저장 할 벡터 변수
				std::vector<cv::Rect> faces;

				// 얼굴인식 템플릿을 이용하여 얼굴인식
				face_classifier.detectMultiScale(
					frameCurrentGrayScale, faces,
					FACE_SEARCH_SCALE,
					MERGE_DETECTED_GROUP_CNTS,
					CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_SCALE_IMAGE,
					cv::Size(FACE_FRAME_WIDTH, FACE_FRAME_HEIGHT)
				);

				int facesSize = faces.size();

				// 침입자의 얼굴을 감지한 경우
				if (facesSize != 0) {
					for (int i = 0; i < facesSize; i++) {
						// 침입자의 얼굴 프레임이 전체 프레임 밖을 벗어나지 않았을 경우에만
						if (
							0 <= faces[i].x
							&& 0 <= faces[i].width
							&& faces[i].x + faces[i].width <= frame.cols
							&& 0 <= faces[i].y
							&& 0 <= faces[i].height
							&& faces[i].y + faces[i].height <= frame.rows
						) {
							// 침입자의 얼굴 부분만 프레임에서 잘라냄
							cv::Mat faceFrame = frame(cv::Rect(faces[i].x, faces[i].y, faces[i].width, faces[i].height));

							// 침입자의 얼굴을 저장 할 디렉토리가 없을 경우 생성
							CreateDirectory(INTRUDER_IMAGES_SAVE_PATH, NULL);

							// 침입자의 얼굴 프레임을 이미지로 저장
							cv::imwrite(INTRUDER_IMAGES_SAVE_PATH + getCurrentTS2Str() + std::string("_") + std::to_string(i) + std::string(".jpg"), faceFrame);

							// 침입자 탐지 메세지를 화면에 출력
							cv::putText(
								frame,
								"Intruder Detected!",
								cv::Point(50, 230),
								CV_FONT_HERSHEY_COMPLEX_SMALL,
								1.0,
								cv::Scalar(0, 0, 255)
							);
						}
					}
				}

				// 움직임은 있었지만 침입자 얼굴을 감지하지 못한 경우
				else {
					// 움직임 감지 메세지를 화면에 출력
					cv::putText(
						frame,
						"Moving Object Detected!",
						cv::Point(20, 230),
						CV_FONT_HERSHEY_COMPLEX_SMALL,
						1.0,
						cv::Scalar(255, 0, 0)
					);
				}
			}
		}

		// 윈도우에 결과 출력
		cv::imshow(WIN_NAME, frame);

		// 현재 프레임을 이전 프레임 저장 변수에 옮김
		frameBeforeGrayScale = frameCurrentGrayScale;

		int keyCode = cv::waitKey(30);

		// esc 키가 눌리면 프레임 캡쳐 종료
		if (keyCode == 27) {
			break;
		}
	}

	return 0;
}