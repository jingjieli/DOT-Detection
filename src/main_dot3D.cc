//////////////////////////////////////////////////////////////////////////////
//																			//
// Copyright 2007 - 2010 Lehrstuhl fuer Informat XVI,						//
// CAMP (Computer Aided Medical Procedures),								//
// Technische Universitaet Muenchen, Germany.								//
//																			//
// All rights reserved.	This file is part of VISION.						//
//																			//
// VISION is free software; you can redistribute it and/or modify it		//
// under the terms of the GNU General Public License as published by		//
// the Free Software Foundation; either version 2 of the License, or		//
// (at your option) any later version.										//
//																			//
// VISION is distributed in the hope that it will be useful, but			//
// WITHOUT ANY WARRANTY; without even the implied warranty of				//
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU			//
// General Public License for more details.									//
//																			//
// You should have received a copy of the GNU General Public License		//
// along with VISION; if not, write to the Free Software Foundation,		//
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA		//
//																			//
// main_dot3D: main_dot3D.cc												//
//																			//
// Authors: Stefan Hinterstoisser 2010										//
// Version: 1.0																//
//																			//
//////////////////////////////////////////////////////////////////////////////

#include "cv_define.h"

#ifdef MAIN_DOT_3D

#include "cv_dot_template.h"
#include "cv_camera.h"
#include "cv_esm.h"

int main(int argc, char * argv[])
{
	const int l_T = 7; // number of pixels in template invariant to translation
	const int l_N = 154 / l_T; // template width in number of regions
	const int l_M = 154 / l_T; // template height in number of regions
	const int l_IN = 640;
	const int l_IM = 480;
	const int l_G = 121; // can be set to a much lower value to deal with changing background 
	// and non-retangular shape of objects 

	// define thresholds 
	int l_learn_thres_up = l_G*0.95;
	//int l_learn_thres_down = l_G*0.9;
	int l_learn_thres_down = l_G*0.90;
	//int l_detect_thres = l_G*0.6;
	int l_detect_thres = l_G*0.85;

	// template size is pre-defined
	cv::cv_dot_template<l_M, l_N, l_T, l_G> l_template(10);

	cv::cv_create_window("hallo1");
	cv::cv_create_window("hallo2");

	std::vector<CvMat*> l_cur_vec;

	cv::cv_camera l_camera;
	cv::cv_timer l_timer1;
	cv::cv_timer l_timer2;
	cv::cv_mouse l_mouse;

	l_mouse.start("hallo1");
	l_camera.set_cam(cv::usb);

	int l_tra_col1 = 0;
	int l_tra_row1 = 0;

	int l_x = -1;
	int l_y = -1;

	int l_class = 0;
	int l_thres = 90;

	bool l_show_esm = false;
	bool l_show_hyp = true;
	bool l_show_rec = true; // show rectangle 
	bool l_learn_onl = false;
	bool l_learn_3D = false;
	bool l_load_prebuilt = false;

	if (l_camera.start_capture_from_cam() == false)
	{
		printf("main_dog: the camera could not be initalized!");
		return 0;
	}

	// enter processing loop
	while (true)
	{
		std::cout << "-------- Processing Loop --------" << std::endl;

		if (l_learn_onl) {
			std::cout << "Online Learning: ENABLE" << std::endl;
		}
		else {
			std::cout << "Online Learning: DISABLE" << std::endl;
		}

		IplImage * lp_color = NULL;
		IplImage * lp_gray = NULL;
		IplImage * lp_mean = NULL;

		lp_color = l_camera.get_image(); // get a new frame
		lp_gray = cv::cv_convert_color_to_gray(lp_color); // convert to gray scale image 
		lp_mean = cv::cv_smooth(lp_gray, 5); // smooth gray scale image

		// get current mouse coordinates
		int l_xx = l_mouse.get_x();
		int l_yy = l_mouse.get_y();
		int l_e = l_mouse.get_event();

		if (l_xx >= 0 && l_yy >= 0)
		{
			l_x = l_xx;
			l_y = l_yy;
		}
		if (l_x != -1 && l_y != -1)
		{
			l_tra_col1 = l_x;
			l_tra_row1 = l_y;
		}

		// use current mouse coordinates as center and make rectangle same size as template image 
		if (l_tra_col1 - l_template.get_width() / 2 >= 0 ||
			l_tra_row1 - l_template.get_height() / 2 >= 0 ||
			l_tra_col1 + l_template.get_width() / 2 <= lp_gray->width - 1 ||
			l_tra_row1 + l_template.get_height() / 2 <= lp_gray->height - 1)
		{
			CvPoint l_pt1;
			CvPoint l_pt2;
			CvPoint l_pt3;
			CvPoint l_pt4;

			l_pt1.x = l_tra_col1 - l_template.get_width() / 2;
			l_pt1.y = l_tra_row1 - l_template.get_height() / 2;
			l_pt2.x = l_tra_col1 + l_template.get_width() / 2;
			l_pt2.y = l_tra_row1 - l_template.get_height() / 2;
			l_pt3.x = l_tra_col1 + l_template.get_width() / 2;
			l_pt3.y = l_tra_row1 + l_template.get_height() / 2;
			l_pt4.x = l_tra_col1 - l_template.get_width() / 2;
			l_pt4.y = l_tra_row1 + l_template.get_height() / 2;

			// draw the rectangle if the option is enabled 
			if (l_show_rec)
			{
				cvLine(lp_color, l_pt1, l_pt2, CV_RGB(0, 0, 0), 3);
				cvLine(lp_color, l_pt2, l_pt3, CV_RGB(0, 0, 0), 3);
				cvLine(lp_color, l_pt3, l_pt4, CV_RGB(0, 0, 0), 3);
				cvLine(lp_color, l_pt4, l_pt1, CV_RGB(0, 0, 0), 3);

				cvLine(lp_color, l_pt1, l_pt2, CV_RGB(255, 255, 0), 1);
				cvLine(lp_color, l_pt2, l_pt3, CV_RGB(255, 255, 0), 1);
				cvLine(lp_color, l_pt3, l_pt4, CV_RGB(255, 255, 0), 1);
				cvLine(lp_color, l_pt4, l_pt1, CV_RGB(255, 255, 0), 1);
			}
		}

		// if a right click of mouse is detected, retrieve current content as template image
		if (l_x != -1 && l_y != -1 && l_e == 2)
		{

			CvMat * lp_result = cvCreateMat(3, 4, CV_32F);
			cvSet(lp_result, cvRealScalar(1)); 

			// access CvMat elements (first two rows)
			// store four polar points coordinates of rectangle
			CV_MAT_ELEM(*lp_result, float, 0, 0) = l_x - l_template.get_width() / 2;
			CV_MAT_ELEM(*lp_result, float, 1, 0) = l_y - l_template.get_height() / 2;
			CV_MAT_ELEM(*lp_result, float, 0, 1) = l_x + l_template.get_width() / 2;
			CV_MAT_ELEM(*lp_result, float, 1, 1) = l_y - l_template.get_height() / 2;
			CV_MAT_ELEM(*lp_result, float, 0, 2) = l_x + l_template.get_width() / 2;
			CV_MAT_ELEM(*lp_result, float, 1, 2) = l_y + l_template.get_height() / 2;
			CV_MAT_ELEM(*lp_result, float, 0, 3) = l_x - l_template.get_width() / 2;
			CV_MAT_ELEM(*lp_result, float, 1, 3) = l_y + l_template.get_height() / 2;

			// for 2D call create_bit_list_fast instead
			//l_template.online_create_bit_list_fast(lp_mean, lp_result, 0, 7, 0.9); // create new template and add a new class
			l_template.online_create_bit_list_fast(lp_mean, lp_result, l_template.get_classes(), 7, 0.9); // create new template and add a new class
			l_template.add_class();

			l_cur_vec.push_back(lp_result); // store the new template image

			l_learn_onl = true; // enable online learning when a template image is captured
			l_class += 1;
		}

		cv::cv_timer l_timer0; // time for entire processing
		cv::cv_timer l_timer1; // time for computing current frame gradient
		cv::cv_timer l_timer2; // time for online learning/processing 
		cv::cv_timer l_timer3;


		l_timer0.start();
		l_timer1.start();
		std::cout << "Call compute_gradients ..." << std::endl;
		std::pair<Ipp8u*, Ipp32f*> l_img = l_template.compute_gradients(lp_mean, 1);
		//std::cout << "l_img.first: " << (int)l_img.first << " " << "l_img.second: " << (int)l_img.second << std::endl;
		l_timer1.stop();
		l_timer2.start();
		std::list<cv::cv_candidate*> * lp_list = NULL;

		// learning online
		// add template with max matching score to the list
		if (l_learn_onl == true)
		{
			std::cout << "online_process ... m_class = " << l_template.get_classes() << std::endl;
			lp_list = l_template.online_process(l_img.first, l_detect_thres, l_IN / l_T, l_IM / l_T);
			//std::cout << "online_process ... lp_list (candidates) size: " << (int)lp_list->size() << std::endl;
		}
		else
		{
			std::cout << "offline_process ... m_class = " << l_template.get_classes() << std::endl;
			lp_list = l_template.process(l_img.first, l_detect_thres, l_IN / l_T, l_IM / l_T);
			//std::cout << "process ... lp_list (candidates) size: " << (int)lp_list->size() << std::endl;
		}
		l_timer2.stop();
		l_timer0.stop();

		for (int i = 0; i < l_template.get_classes(); i++) {
			std::cout << "class " << i << " has " << lp_list[i].size() << " candidates." << std::endl;
		}

		// there're caniddate in the list
		if (lp_list != NULL && l_template.get_classes() != 0)
		{
			std::cout << "candidate list is not empty ..." << std::endl;
			int l_size = 0; // to store the total number of candidate(s) from all class(es)

			float * lp_max_val = new float[l_template.get_classes()];
			float * lp_res_val = new float[l_template.get_classes()];
			float * lp_ind_val = new float[l_template.get_classes()];
			float * lp_col_val = new float[l_template.get_classes()];
			float * lp_row_val = new float[l_template.get_classes()];

			//std::cout << "Number of classes, l_template.get_classes(): " << l_template.get_classes() << std::endl;

			// iterate over each class, for each class, only consider the candidate with the largest score (already sorted into descending)
			for (int l_j = 0; l_j < l_template.get_classes(); ++l_j)
			{
				//std::cout << "------ START OUTTER LOOP ------" << std::endl;
				// process candidates belonging to each class
				int l_counter = 0;

				int l_end_counter = 0;

				if (l_learn_onl == true)
				{
					l_end_counter = 1;
				}
				else
				{
					l_end_counter = 7;
				}
				lp_max_val[l_j] = 0; // max value of current class
				lp_res_val[l_j] = -1; // result value of current class
				lp_ind_val[l_j] = 0; // index value of current class

				//std::cout << "l_size: " << l_size << "; lp_list[l_j] size: " << lp_list[l_j].size() << std::endl;
				l_size += lp_list[l_j].size(); // accumulate candidates number
				std::cout << lp_list[l_j].size() << " candidates added." << std::endl;
				//std::cout << "l_size: " << l_size << std::endl;

				lp_list[l_j].sort(cv::cv_candidate_ptr_cmp()); // sort candidates of a class into descending order according to their m_val

				for (std::list<cv::cv_candidate*>::iterator l_i = lp_list[l_j].begin(); l_i != lp_list[l_j].end(); ++l_i)
					// iterate over all candidates in a certain class
				{
					//std::cout << "------ START INNER LOOP ------" << std::endl;
					if (l_counter < l_end_counter)
					{
						CvMat * lp_rec = cvCreateMat(3, 4, CV_32F);
						cvCopy(l_template.get_rec()[(*l_i)->m_ind - 1], lp_rec);

						CV_MAT_ELEM(*lp_rec, float, 0, 0) += (*l_i)->m_col;
						CV_MAT_ELEM(*lp_rec, float, 1, 0) += (*l_i)->m_row;
						CV_MAT_ELEM(*lp_rec, float, 0, 1) += (*l_i)->m_col;
						CV_MAT_ELEM(*lp_rec, float, 1, 1) += (*l_i)->m_row;
						CV_MAT_ELEM(*lp_rec, float, 0, 2) += (*l_i)->m_col;
						CV_MAT_ELEM(*lp_rec, float, 1, 2) += (*l_i)->m_row;
						CV_MAT_ELEM(*lp_rec, float, 0, 3) += (*l_i)->m_col;
						CV_MAT_ELEM(*lp_rec, float, 1, 3) += (*l_i)->m_row;

						if (l_counter == 0) // the first candidate has the largest value because the list is already in descending order
						{
							if ((*l_i)->m_val >= l_detect_thres) // current (the first) candidate value is greater than the pre-defined detecting threshold 
							{
								l_template.render(lp_color, l_template.get_cnt()[(*l_i)->m_ind - 1], (*l_i)->m_row, (*l_i)->m_col); // display contour 

								// print debug info to the console
								std::cout << "CLASS " << l_j << " CANDIDATE (index) " << (*l_i)->m_ind << " C_CLASS: " << (*l_i)->m_cla << " SCORE: " << (*l_i)->m_val <<
									" CLUSTER: " << (*l_i)->m_clu << " ROWS: " << (*l_i)->m_row << " COLS: " << (*l_i)->m_col << std::endl;
							}
							lp_max_val[l_j] = (*l_i)->m_val; // store the matching score for current class

							if (l_show_hyp == true)
							{
								cv::cv_draw_poly(lp_color, lp_rec, 3, 255, 255, 255);
								cv::cv_draw_poly(lp_color, lp_rec, 1, 0, 0, 0);
							}
							if (l_learn_onl == true) // learn a new template online
							{
								if (lp_max_val[l_j] >= l_learn_thres_down &&
									lp_max_val[l_j] < l_learn_thres_up)
								{
									//std::cout << "Threshold value in range: " << lp_max_val[l_j] << " Call online_create_bit_list_fast ..." << std::endl;
									float l_center_x = 0;
									float l_center_y = 0;

									float l_weight = 0;

									int l_off = 40;

									for (int l_r = 0; l_r < l_M*l_T + l_off * 2; ++l_r)
									{
										for (int l_c = 0; l_c < l_N*l_T + l_off * 2; ++l_c)
										{
											int l_col = l_c + (*l_i)->m_col - l_off;
											int l_row = l_r + (*l_i)->m_row - l_off;

											if (l_col >= 0 && l_row >= 0 &&
												l_col < lp_gray->width &&
												l_row < lp_gray->height)
											{
												float l_mag = *(l_img.second + l_row*lp_gray->width + l_col);

												if (l_mag > 10)
												{
													l_center_x += l_mag*l_col;
													l_center_y += l_mag*l_row;

													l_weight += l_mag;
												}
											}
										}
									}
									l_center_x /= l_weight;
									l_center_y /= l_weight;

									CvMat * lp_rec1 = cvCreateMat(3, 4, CV_32F);
									cvSet(lp_rec1, cvRealScalar(1));

									CV_MAT_ELEM(*lp_rec1, float, 0, 0) = l_center_x - l_N*l_T / 2;
									CV_MAT_ELEM(*lp_rec1, float, 1, 0) = l_center_y - l_M*l_T / 2;
									CV_MAT_ELEM(*lp_rec1, float, 0, 1) = l_center_x + l_N*l_T / 2;
									CV_MAT_ELEM(*lp_rec1, float, 1, 1) = l_center_y - l_M*l_T / 2;
									CV_MAT_ELEM(*lp_rec1, float, 0, 2) = l_center_x + l_N*l_T / 2;
									CV_MAT_ELEM(*lp_rec1, float, 1, 2) = l_center_y + l_M*l_T / 2;
									CV_MAT_ELEM(*lp_rec1, float, 0, 3) = l_center_x - l_N*l_T / 2;
									CV_MAT_ELEM(*lp_rec1, float, 1, 3) = l_center_y + l_M*l_T / 2;

									l_template.online_create_bit_list_fast(lp_mean, lp_rec, l_j, 7, 0.9); // create a new template and add a new class
									cvReleaseMat(&lp_rec1);
								}
							}
						}
						else // l_counter != 0 (discard other candidates)
						{
							if (l_show_hyp == true)
							{
								cv::cv_draw_poly(lp_color, lp_rec, 2, 0, 0, 0);
							}
						}
						cvReleaseMat(&lp_rec);
					}
					else // l_counter > l_end_counter
					{
						break;
					}
					++l_counter;
					//std::cout << "------ END INNER LOOP ------" << std::endl;
				}
				//std::cout << "------ END OUTTER LOOP ------" << std::endl;
			}


			for (int l_i = 0; l_i < l_template.get_classes(); ++l_i)
			{
				cv::empty_ptr_list(lp_list[l_i]);
			}

			//std::cerr << "!NULL; ";
			//std::cerr << "Num of pts, m_Rec " << l_template.get_rec().size() << " Num of cnts, m_cnt " << l_template.get_cnt().size() << " ";
			std::cerr << "tim: " << (int)(l_timer0.get_time() * 1000) << "ms; fps: " << (int)l_timer0.get_fps() << "; ";
			std::cerr << "pre: " << (int)(l_timer1.get_time() * 1000) << "ms; pro: " << (int)(l_timer2.get_time() * 1000) << "ms; ";

			for (int l_i = 0; l_i < l_template.get_classes(); ++l_i)
			{
				std::cerr << "class " << l_i << " max value: " << (int)(lp_max_val[l_i]) << std::endl;
			}
			//std::cerr << "templates num: " << l_template.get_templates() << " candidates num: " << l_size << " classes num: " << l_template.get_classes() << " " << char(13) << std::flush;
			std::cerr << "templates: " << l_template.get_templates() << " candidates: " << l_size << " classes: " << l_template.get_classes() << " " << char(13) << std::endl;

			delete[] lp_max_val;
			delete[] lp_res_val;
			delete[] lp_row_val;
			delete[] lp_col_val;
			delete[] lp_ind_val;
			delete[] lp_list;
		}
		else
		{
			std::cerr << "candidate list is empty ..." << std::endl;
			std::cerr << "tim: " << (int)(l_timer0.get_time() * 1000) << "ms; fps: " << (int)l_timer0.get_fps() << "; ";
			std::cerr << "pre: " << (int)(l_timer1.get_time() * 1000) << "ms; pro: " << (int)(l_timer2.get_time() * 1000) << "ms; ";
			//std::cerr << "templates num: " << l_template.get_templates() << "    " << char(13) << std::flush;
			std::cerr << "templates: " << l_template.get_templates() << "    " << char(13) << std::endl;
		}


		cv::cv_show_image(lp_color, "hallo1");
		cv::cv_show_image(lp_mean, "hallo2");
		int l_key = cvWaitKey(1);

		ippsFree(l_img.first);
		ippsFree(l_img.second);

		if (l_key == 2621440) // when down-arrow is pressed
		{
			l_thres--;

			if (l_thres < -100)
			{
				l_thres = -100;
			}
			std::cerr << "new threshold: " << l_thres << "  " << std::endl;
		}

		if (l_key == 2490368) // when up-arrow is pressed
		{
			l_thres++;

			if (l_thres > +100)
			{
				l_thres = +100;
			}
			std::cerr << "new threshold: " << l_thres << "  " << std::endl;
		}

		if (l_key == 106) // when button "j" is pressed 
		{
			static int l_n = 0;

			std::stringstream l_name;

			l_name << "..\\pics\\eccv10\\screenshots\\img_" << l_n << ".png";

			cv::cv_save_image(l_name.str(), lp_color);

			++l_n;
		}

		if (l_key == 114) // when button "r" is pressed
		{
			l_show_rec = !l_show_rec;
		}

		if (l_key == 104) // when button "h" is pressed 
		{
			l_show_hyp = !l_show_hyp;
		}

		if (l_key == 115) // when button "s" is pressed
		{
			std::string file_name = "..\\results\\dot_model";
			l_template.save(file_name);
		}

		if (l_key == 108) // when button "l" is pressed
		{
			std::string file_name = "..\\results\\dot_model";
			l_template.load(file_name);
			//l_template.clear_clu_list();
			//l_template.cluster_heu(4);
			l_template.load_model_helper();
		}

		if (l_key == 101) // when button "e" is pressed
		{
			l_show_esm = !l_show_esm;
		}

		if (l_key == 105) // when button "i" is pressed
		{
			l_learn_onl = !l_learn_onl;

			if (l_learn_onl == false)
			{
				l_template.clear_clu_list();
				l_template.cluster_heu(4);
			}
		}

		if (l_key == 100) // when button "d" is pressed
		{
			for (int l_i = 0; l_i < l_template.get_classes(); ++l_i)
			{
				cvReleaseMat(&l_cur_vec[l_i]);
			}
			l_cur_vec.clear();

			l_template.clear_clu_list();
			l_template.clear_bit_list();
			l_template.clear_rec_list();
			l_template.clear_cnt_list();
		}

		if (l_key == 27) // when "ESC" is pressed
		{
			for (int l_i = 0; l_i < l_template.get_classes(); ++l_i)
			{
				cvReleaseMat(&l_cur_vec[l_i]);
			}
			l_cur_vec.clear();

			cvReleaseImage(&lp_color);
			cvReleaseImage(&lp_gray);
			cvReleaseImage(&lp_mean);

			l_template.clear_clu_list();
			l_template.clear_bit_list();
			l_template.clear_rec_list();
			l_template.clear_cnt_list();

			return(0);
		}
		cvReleaseImage(&lp_color);
		cvReleaseImage(&lp_gray);
		cvReleaseImage(&lp_mean);
	}

	return 0;
}

#endif

