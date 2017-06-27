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

	Lesson(vector<float> position, int field_w, int field_h, int movei, int movej, vector<float> priorities) :
		field_w(field_w), field_h(field_h),
		movei(movei), movej(movej),
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

	Lesson rotateClockwise() const
	{
		Lesson res(*this);
		if (field_w != field_h)
		{
			throw runtime_error("Can't rotate a non-square field");
		}

		int fd = field_w;

		for (int j = 0; j < fd; j++) {
			for (int i = 0; i < fd; i++) {
				res.position[fd * i + (fd - 1 - j)] = position[j * field_w + i];
				res.priorities[fd * i + (fd - 1 - j)] = priorities[j * field_w + i];
			}
		}

		res.movei = fd - 1 - movej;
		res.movej = movei;

		return res;
	}

	Lesson mirrorHorizontal() const
	{
		Lesson res(*this);

		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				res.position[j * field_w + i] = res.position[j * field_w + (field_w - 1 - i)];
				res.priorities[j * field_w + i] = res.priorities[j * field_w + (field_w - 1 - i)];
			}
		}

		res.movei = field_w - 1 - movei;
		res.movej = movej;

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

// mean-squared-error loss function for regression

class mse_priorities {
public:
	static float f(const vec_t &y, const vec_t &t) {
		assert(y.size() == t.size());
		float d{ 0.0 };

		for (size_t i = 0; i < y.size(); ++i)
		{
			d += t[i] * (y[i] - t[i]) * (y[i] - t[i]);
		}

		return d / static_cast<float>(y.size());
	}

	static vec_t df(const vec_t &y, const vec_t &t) {
		assert(y.size() == t.size());
		vec_t d(t.size());
		float factor = float(2) / static_cast<float>(t.size());

		for (size_t i = 0; i < y.size(); ++i)
		{
			d[i] = factor * t[i] * (y[i] - t[i]);
		}

		return d;
	}
};

void makeMove(network<sequential> net, vector<float> field_data, int field_w, int field_h, int& movei, int& movej, int& victor)
{
	vec_t pos_item;
	for (int j = 0; j < field_h; j++) {
		for (int i = 0; i < field_w; i++) {
			pos_item.push_back(field_data[j * field_w + i]);
		}
	}

	vec_t result = net.predict(pos_item);

	// Searching for the maximum
	bool occupied;
	float maxpriority;
	do
	{
		occupied = false;
		movei = -1; movej = -1;
		maxpriority = -1.0;
		for (int j = 0; j < field_h; j++)
		{
			for (int i = 0; i < field_w; i++)
			{
				if (result[j * field_w + i] > maxpriority)
				{
					maxpriority = result[j * field_w + i];
					movei = i;
					movej = j;

					if (abs(field_data[j * field_w + i]) > 0.5)
					{
						occupied = true;
						result[j * field_w + i] = -1.0; // clearing this priority
						break;
					}

				}
			}
		}
		if (maxpriority == -1.0)
		{
			victor = -1;
			printf("Draw\n");
			break;
		}
	} while (occupied);
}

void train(int field_w, int field_h, network<sequential> net, float mse_stop)
{
	vector<Lesson> usefulLessons;

	// Loading lessons
	ifstream lessonsFile;
	lessonsFile.open("lessons.dat", ios::app | ios::binary | ios::in);
	while (!lessonsFile.eof())
	{
		char magic = ' ';
		lessonsFile.read(&magic, 1);
		if (magic != 'L') break;

		int movei;
		lessonsFile.read((char*)&movei, 4);
		int movej;
		lessonsFile.read((char*)&movej, 4);

		vector<float> position;
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				float f;
				lessonsFile.read((char*)&f, 4);
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

		usefulLessons.push_back(Lesson(position, field_w, field_h, movei, movej, priorities));
	}
	lessonsFile.close();

	printf("Lessons in the book: %d\n", usefulLessons.size());

	int training_batch = usefulLessons.size();
	int batches_count = 1;

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
	double loss = 0; int ee = 0;
	
	double delta_loss_per_epoch;
	
	//gradient_descent opt; opt.alpha = 0.75;
	adam opt;
	int succeeded_tests;

	size_t epochs = 200;
	loss = net.get_loss<mse>(train_input_data, train_output_data);

	do
	{
		net.fit<mse>(opt, train_input_data, train_output_data, batch_size, epochs);

		double old_loss = loss;
		loss = net.get_loss<mse>(train_input_data, train_output_data);

		delta_loss_per_epoch = (old_loss - loss) / epochs;
		//if (delta_loss_per_epoch < 0) opt.alpha /= 1.5;

		// Scoring

		succeeded_tests = 0;
		for (int i = 0; i < usefulLessons.size(); i++)
		{
			vector<float> field_data = usefulLessons[i].position;

			int movei, movej, victor;
			makeMove(net, field_data, field_w, field_h, movei, movej, victor);

			if (movei == usefulLessons[i].movei &&
				movej == usefulLessons[i].movej)
			{
				succeeded_tests++;
			}
		}
		
		ee += epochs;
		cout << "epoch " << ee << ": loss=" << loss << " dloss=" << delta_loss_per_epoch << "; learned : " << succeeded_tests << " of " << usefulLessons.size() << endl;

	} while (/*succeeded_tests < usefulLessons.size()*/ abs(delta_loss_per_epoch) > mse_stop);
}

int main(int argc, char** argv)
{
	const int field_w = 7, field_h = 7, vic_line_len = 4;

	network<sequential> net;
	try
	{
		net.load("xoxonet.weights");
		printf("Net loaded\n");
	}
	catch (nn_error&)
	{
		printf("Can't load the net. Creating a new one\n");

		int size = field_w * field_h;

/*		int maps = 50;

		net <<
			layers::conv(field_w, field_h, 3, 1, maps) <<
			layers::fc(maps, 2 * maps) << tanh_layer(2 * maps) <<
			layers::fc(2 * maps, 2 * maps) << tanh_layer(2 * maps) <<
			layers::fc(2 * maps, maps) << tanh_layer(maps) <<
			layers::deconv(1, 1, 3, maps, 1);

			420 of 736
			*/

		int conv_out_w = field_w - vic_line_len + 1;
		int conv_out_h = field_h - vic_line_len + 1;
		int conv_out = conv_out_w * conv_out_h;

		int maps = conv_out_w * conv_out_h * 2;	// from the ceiling

		net <<
			layers::conv(field_w, field_h, vic_line_len, 1, maps) <<
			layers::fc(    conv_out * maps, 2 * conv_out * maps) << tanh_layer(2 * conv_out * maps) <<
			layers::fc(2 * conv_out * maps, 2 * conv_out * maps) << tanh_layer(2 * conv_out * maps) <<
			layers::fc(2 * conv_out * maps,     conv_out * maps) << tanh_layer(    conv_out * maps) <<
			layers::deconv(conv_out_w, conv_out_h, vic_line_len, maps, 1);


	}

	if (argc == 2 && strcmp(argv[1], "train-first") == 0)
	{
		train(field_w, field_h, net, 1e-4);
	}


	vector<Lesson> lessons[2];	// Each player's lessons

	vector<float> field_data;
	for (int j = 0; j < field_h; j++) {
		for (int i = 0; i < field_w; i++) {
			field_data.push_back(0.0);
		}
	}

	srand(time(nullptr));
	int userPlayerIndex = rand() % 2;

	int victor = -2;
	do {
		for (int pI = 0; pI < 2; pI++) {
			printField(field_data, field_w, field_h);
			float vic = checkVictory(field_data, field_w, field_h, vic_line_len);
			if (vic > 0.5)
			{
				printf("Player X wins\n");
				victor = 0;
				break;
			}
			else if (vic < -0.5)
			{
				printf("Player O wins\n");
				victor = 1;
				break;
			}

			vector<float> field_data_me_him = field_data;
			if (userPlayerIndex != 1) {
				// If the user plays "X", we inverse the field because "X" should mean "me"
				for (int j = 0; j < field_h; j++) {
					for (int i = 0; i < field_w; i++) {
						field_data_me_him[j * field_w + i] = -field_data_me_him[j * field_w + i];
					}
				}
			}

			int movei, movej;
			if (pI == userPlayerIndex)
			{
				// Player move
				do {
					printf("%c>", playerSymbol(pI));
					char x = getchar();
					char y = getchar();
					char c;  while ((c = getchar()) != '\n' && c != EOF) {}
					movei = x - 'a';
					movej = y - '1';
				} while (movei >= field_w || movej > field_h || movei < 0 || movej < 0);

			}
			else
			{
				// AI move
				makeMove(net, field_data_me_him, field_w, field_h, movei, movej, victor);
			}

			if (victor != -1) {
				// If not draw
				lessons[pI].push_back(Lesson(field_data_me_him, field_w, field_h, movei, movej, 1.0));
				field_data[movej * field_w + movei] = playerVal(pI);
			}
		}

	} while (victor < -1);

	if (victor == userPlayerIndex) {
		printf("Other player wins. I shall learn\n");

		vector<Lesson> usefulLessons;
		float priority = 1.0;
		for (int k = lessons[victor].size() - 1; k >= 0; k--)
		{
			Lesson cur = lessons[victor][k].mulPriority(priority);
			
			// In the book 1 means "me" and -1 means "other player", not "X" and "O"
			if (userPlayerIndex != 1) cur = cur.inverse();

			usefulLessons.push_back(cur);
			Lesson curr1 = cur.rotateClockwise();
			usefulLessons.push_back(curr1);
			Lesson curr2 = curr1.rotateClockwise();
			usefulLessons.push_back(curr2);
			Lesson curr3 = curr2.rotateClockwise();
			usefulLessons.push_back(curr3);

			Lesson curm = cur.mirrorHorizontal();
			usefulLessons.push_back(curm);
			Lesson curmr1 = curm.rotateClockwise();
			usefulLessons.push_back(curmr1);
			Lesson curmr2 = curmr1.rotateClockwise();
			usefulLessons.push_back(curmr2);
			Lesson curmr3 = curmr2.rotateClockwise();
			usefulLessons.push_back(curmr3);

			Lesson curi = cur.inverse().mulPriority(0.75);		// Defense is less prioritized than attack
			usefulLessons.push_back(curi);
			Lesson curir1 = curi.rotateClockwise();
			usefulLessons.push_back(curir1);
			Lesson curir2 = curir1.rotateClockwise();
			usefulLessons.push_back(curir2);
			Lesson curir3 = curir2.rotateClockwise();
			usefulLessons.push_back(curir3);

			Lesson curim = curi.mirrorHorizontal();
			usefulLessons.push_back(curim);
			Lesson curimr1 = curim.rotateClockwise();
			usefulLessons.push_back(curimr1);
			Lesson curimr2 = curimr1.rotateClockwise();
			usefulLessons.push_back(curimr2);
			Lesson curimr3 = curimr2.rotateClockwise();
			usefulLessons.push_back(curimr3);
			
			priority /= 2.0;
		}

		// Saving useful lessons to file
		ofstream lessonsFile;
		lessonsFile.open("lessons.dat", ios::app | ios::binary | ios::out);
		for (int k = 0; k < usefulLessons.size(); k++)
		{
			lessonsFile.write("L", 1);	// Magic for a lesson

			lessonsFile.write((const char*) &(usefulLessons[k].movei), 4);
			lessonsFile.write((const char*) &(usefulLessons[k].movej), 4);
			for (int j = 0; j < field_h; j++) {
				for (int i = 0; i < field_w; i++) {
					lessonsFile.write((const char*) &(usefulLessons[k].position[j * field_w + i]), 4);
				}
			}
			for (int j = 0; j < field_h; j++) {
				for (int i = 0; i < field_w; i++) {
					lessonsFile.write((const char*) &(usefulLessons[k].priorities[j * field_w + i]), 4);
				}
			}
		}
		lessonsFile.close();

		printf("%d lessons appended to the book\n", usefulLessons.size());

		train(field_w, field_h, net, 1e-4);

		printf("Saving net...");
		net.save("xoxonet.weights");
	}
	else if (victor == -1)
	{
		printf("Nothing to learn from draw");
	}
	else
	{
		printf("I win. I am a clever bot. No more learning today\n");
	}

    return 0;
}
