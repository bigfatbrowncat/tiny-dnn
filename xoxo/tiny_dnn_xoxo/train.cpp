#include <stdio.h>

#include "tiny_dnn/tiny_dnn.h"

#include <memory>
#include <vector>
#include <fstream>

using namespace tiny_dnn;

using namespace std;

void printField(vector<float> field_data, int field_w, int field_h) {
	printf("   ");
	for (int i = 0; i < field_w; i++)
	{
		printf("%c ", 'a' + i);
	}
	printf("\n");

	for (int j = 0; j < field_h; j++)
	{
		printf("%2d", j + 1);
		for (int i = 0; i < field_w; i++)
		{

			if (field_data[j * field_w + i] > 0.5) {
				printf(" X");
			}
			else if (field_data[j * field_w + i] < -0.5)
			{
				printf(" O");
			}
			else
			{
				printf(" .");
			}
			//field_data[j * field_w + i]
		}
		printf("\n");
	}
	printf("\n");

}

bool playerCond(int pI, float val) {
	if (pI == 0 /*X*/) return val > 0.5;
	else if (pI == 1 /*O*/) return val < -0.5;
	else throw 0;
}

float playerVal(int pI) {
	if (pI == 0 /*X*/) return 1.0;
	else if (pI == 1 /*O*/) return -1.0;
	else throw 0;
}

char playerSymbol(int pI) {
	if (pI == 0 /*X*/) return 'X';
	else if (pI == 1 /*O*/) return 'O';
	else throw 0;
}

float checkVictory(vector<float> field_data, int field_w, int field_h, int line_len) {
	for (int j = 0; j < field_h; j++) {
		for (int i = 0; i < field_w; i++) {

			// Iterate thru players - 0=X 1=O
			for (int pI = 0; pI < 2; pI++) {
				int cnt;
				// Checking player victory
				if (playerCond(pI, field_data[j * field_w + i])) {

					// Right
					cnt = 1;
					for (int p = 1; p < line_len; p++) {
						if (i + p < field_w) {
							if (playerCond(pI, field_data[j * field_w + (i + p)])) cnt++;
						}
					}
					if (cnt == line_len)
						return playerVal(pI);

					// Down
					cnt = 1;
					for (int q = 1; q < line_len; q++) {
						if (j + q < field_h) {
							if (playerCond(pI, field_data[(j + q) * field_w + i])) cnt++;
						}
					}
					if (cnt == line_len) 
						return playerVal(pI);

					// Right-down
					cnt = 1;
					for (int pq = 1; pq < line_len; pq++) {
						if (i + pq < field_w && j + pq < field_h) {
							if (playerCond(pI, field_data[(j + pq) * field_w + (i + pq)])) cnt++;
						}
					}
					if (cnt == line_len)
						return playerVal(pI);

					// Left-down
					cnt = 1;
					for (int pq = 1; pq < line_len; pq++) {
						if (i - pq >= 0 && j + pq < field_h) {
							if (playerCond(pI, field_data[(j + pq) * field_w + (i - pq)])) cnt++;
						}
					}
					if (cnt == line_len)
						return playerVal(pI);
				}
			}

		}
	}
	return 0.0; // No victory found
}

class Lesson
{
public:
	int field_w, field_h;
	vector<float> position, priorities;
	int movei, movej;

	Lesson(vector<float> position, int field_w, int field_h, int movei, int movej, float priority) :
		field_w(field_w), field_h(field_h),
		movei(movei), movej(movej),
		position(position)
	{
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				priorities.push_back(i == movei && j == movej ? priority : 0.0);
			}
		}
	}

	Lesson(vector<float> position, int field_w, int field_h, vector<float> priorities) :
		field_w(field_w), field_h(field_h),
		position(position), priorities(priorities)
	{

	}

	Lesson inverse() const
	{
		Lesson res(*this);

		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				res.position[j * field_w + i] *= -1;
			}
		}

		return res;
	}

	Lesson mulPriority(float priority_mul) const
	{
		Lesson res(*this);

		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				res.priorities[j * field_w + i] *= priority_mul;
			}
		}

		return res;
	}

};

int main()
{
	const int field_w = 3, field_h = 3;

	network<sequential> net;

	try
	{
		net.load("xoxonet.weights");
		printf("Net loaded\n");
	}
	catch (nn_error&)
	{
		printf("Can't load the net. Creating a new one\n");

		net << layers::fc(field_w * field_h, field_w * field_h * 3) << tanh_layer(field_w * field_h * 3) <<
			layers::fc(field_w * field_h * 3, field_w * field_h * 3) << tanh_layer(field_w * field_h * 3) <<
			layers::fc(field_w * field_h * 3, field_w * field_h * 3) << tanh_layer(field_w * field_h * 3) <<
			layers::fc(field_w * field_h * 3, field_w * field_h);

	}

	vector<Lesson> usefulLessons;

	// Loading lessons
	ifstream lessonsFile;
	lessonsFile.open("lessons.dat", ios::app | ios::binary | ios::in);
	while (!lessonsFile.eof())
	{
		vector<float> position;
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				float f;
				lessonsFile.read((char*) &f, 4);
				position.push_back(f);
			}
		}

		vector<float> priorities;
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				float f;
				lessonsFile.read((char*)&f, 4);
				priorities.push_back(f);
			}
		}

		usefulLessons.push_back(Lesson(position, field_w, field_h, priorities));
	}
	lessonsFile.close();

	int training_batch = usefulLessons.size();
	int batches_count = 2;

	// 1. Generating training & testing data

	vector<vec_t> train_input_data;
	vector<vec_t> train_output_data;

	for (int i = 0; i < batches_count * training_batch; ++i)
	{
		int k = i % usefulLessons.size();

		vec_t pos_item;
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				pos_item.push_back(usefulLessons[k].position[j * field_w + i]);
			}
		}
		train_input_data.push_back(pos_item);

		vec_t pri_item;
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				pri_item.push_back(usefulLessons[k].priorities[j * field_w + i]);
			}
		}
		train_output_data.push_back(pri_item);
	}


	printf("Training...\n");

    size_t batch_size = training_batch;
	for (int ee = 0; ee < 20; ee++) {
		size_t epochs = 100;
		gradient_descent opt;
		net.fit<mse>(opt, train_input_data, train_output_data, batch_size, epochs);

		double loss = net.get_loss<mse>(train_input_data, train_output_data);
		cout << ee+1 << " mse : " << loss << endl;
	}

	printf("Saving net...");
	net.save("xoxonet.weights");
    
	return 0;
}