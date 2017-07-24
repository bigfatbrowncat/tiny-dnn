#pragma once

#include <cassert>

#include <vector>
#include <tiny_dnn/tiny_dnn.h>

class Table
{
public:
	int width, height, channels;
	std::vector<float> vals;

	static Table empty(int width, int height, int channels) {
		std::vector<float> vals(width * height * channels, 0.0);
		return Table(vals, width, height, channels);
	}
	static Table ones(int width, int height, int channels) {
		std::vector<float> vals(width * height * channels, 1.0);
		return Table(vals, width, height, channels);
	}

	Table(std::vector<float> vals, int width, int height, int channels) : vals(vals), width(width), height(height), channels(channels) { }
	Table(int width, int height, int channels) : width(width), height(height), channels(channels) { }

	Table inverseChannels() const
	{
		assert(channels == 2);

		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				float tmp = res.vals[(j * width + i) * 2 + 0];
				res.vals[(j * width + i) * 2 + 0] = res.vals[(j * width + i) * 2 + 1];
				res.vals[(j * width + i) * 2 + 1] = tmp;

			}
		}

		return res;
	}

	Table rotateClockwise() const
	{
		Table res(*this);
		assert(width == height);

		int fd = width;

		for (int j = 0; j < fd; j++) {
			for (int i = 0; i < fd; i++) {
				for (int c = 0; c < channels; c++) {
					res.vals[(fd * i + (fd - 1 - j)) * channels + c] = vals[(j * fd + i) * channels + c];
				}
			}
		}

		return res;
	}

	Table mirrorHorizontal() const
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				for (int c = 0; c < channels; c++) {
					res.vals[(j * width + i) * channels + c] = vals[(j * width + (width - 1 - i)) * channels + c];
				}
			}
		}

		return res;
	}

	Table multiply(float priority_mul) const
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				for (int c = 0; c < channels; c++) {
					res.vals[(j * width + i) * channels + c] *= priority_mul;
				}
			}
		}

		return res;
	}

	/*Table translateRoll(int dx, int dy)
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {

				int i2 = (i + dx + width) % width;
				int j2 = (j + dy + height) % height;

				for (int c = 0; c < channels; c++) {
					res.vals[(j2 * width + i2) * channels + c] = vals[(j * width + i) * channels + c];
				}
			}
		}

		return res;
	}*/

	Table extendToroidal(int depth)
	{
		std::vector<float> resData;
		for (int j = -depth; j < height + 2 * depth; j++) {
			for (int i = -depth; i < width + 2 * depth; i++) {
				for (int c = 0; c < channels; c++) {
					int id = (i + width) % width;
					int jd = (j + height) % height;

					resData.push_back(vals[(jd * width + id) * channels + c]);
				}
			}
		}
		return Table(resData, width + 2 * depth, height + 2 * depth, channels);
	}

	tiny_dnn::vec_t toVec() {
		tiny_dnn::vec_t pos_item;
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				for (int c = 0; c < channels; c++) {
					pos_item.push_back(vals[(j * width + i) * channels + c]);
				}
			}
		}
		return pos_item;
	}

	static Table max(const Table& left, const Table& right) {
		Table res(left);

		for (int j = 0; j < res.height; j++) {
			for (int i = 0; i < res.width; i++) {
				for (int c = 0; c < res.channels; c++) {
					res.vals[(j * res.width + i) * res.channels + c] =
						std::max(
							left.vals[(j * res.width + i) * res.channels + c],
							right.vals[(j * res.width + i) * res.channels + c]
						);
				}
			}
		}

		return res;
	}

	static Table fromVec(tiny_dnn::vec_t v, int width, int height, int channels) {
		std::vector<float> vals(width * height * channels);
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				for (int c = 0; c < channels; c++) {
					vals[(j * width + i) * channels + c] = v[(j * width + i) * channels + c];
				}
			}
		}
		return Table(vals, width, height, channels);
	}

	static Table score(tiny_dnn::network<tiny_dnn::sequential> net, Table field);
};
