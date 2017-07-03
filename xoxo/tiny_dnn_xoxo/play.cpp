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
	for (int j = 0; j < field_h + line_len - 1; j++) {
		for (int i = 0; i < field_w + line_len - 1; i++) {
			int i1 = i % field_w;
			int j1 = j % field_h;

			// Iterate thru players - 0=X 1=O
			for (int pI = 0; pI < 2; pI++) {
				int cnt;
				// Checking player victory
				if (playerCond(pI, field_data[j1 * field_w + i1])) {

					// Right
					cnt = 1;
					for (int p = 1; p < line_len; p++) {
						int i2 = (i + p) % field_w;
						if (playerCond(pI, field_data[j1 * field_w + i2])) cnt++;
					}
					if (cnt == line_len)
						return playerVal(pI);

					// Down
					cnt = 1;
					for (int q = 1; q < line_len; q++) {
						int j2 = (j + q) % field_h;
						if (playerCond(pI, field_data[j2 * field_w + i1])) cnt++;
					}
					if (cnt == line_len) 
						return playerVal(pI);

					// Right-down
					cnt = 1;
					for (int pq = 1; pq < line_len; pq++) {
						int i2 = (i + pq) % field_w;
						int j2 = (j + pq) % field_h;
						if (playerCond(pI, field_data[j2 * field_w + i2])) cnt++;
					}
					if (cnt == line_len)
						return playerVal(pI);

					// Left-down
					cnt = 1;
					for (int pq = 1; pq < line_len; pq++) {
						int i2 = (i - pq + field_w) % field_w;
						int j2 = (j + pq) % field_h;
						if (playerCond(pI, field_data[j2 * field_w + i2])) cnt++;
					}
					if (cnt == line_len)
						return playerVal(pI);
				}
			}

		}
	}
	return 0.0; // No victory found
}

class Point {
public:
	int i, j, width, height;

	Point(int i, int j, int width, int height) : i(i), j(j), width(width), height(height) { }

	Point rotateClockwise() const
	{
		Point res(*this);
		assert(width == height);

		res.i = width - 1 - j;
		res.j = i;

		return res;
	}

	Point mirrorHorizontal() const
	{
		Point res(*this);

		res.i = width - 1 - i;
		res.j = j;

		return res;
	}

	/*Point translateRoll(int dx, int dy)
	{
		Point res(*this);
		res.i = (i + dx + width) % width;
		res.j = (j + dy + height) % height;
		return res;
	}*/
};

class Table
{
public:
	int width, height;
	vector<float> vals;

	Table(vector<float> vals, int width, int height) : vals(vals), width(width), height(height) { }
	Table(int width, int height) : width(width), height(height) { }

	Table inverse() const
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				res.vals[j * width + i] *= -1;
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
				res.vals[fd * i + (fd - 1 - j)] = vals[j * fd + i];
			}
		}

		return res;
	}

	Table mirrorHorizontal() const
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				res.vals[j * width + i] = vals[j * width + (width - 1 - i)];
			}
		}

		return res;
	}

	Table multiply(float priority_mul) const
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				res.vals[j * width + i] *= priority_mul;
			}
		}

		return res;
	}

	Table translateRoll(int dx, int dy)
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {

				int i2 = (i + dx + width) % width;
				int j2 = (j + dy + height) % height;

				res.vals[j2 * width + i2] = vals[j * width + i];
			}
		}

		return res;
	}
};

Table max(Table one, Table two)
{
	assert(one.width == two.width);
	assert(one.height == two.height);

	vector<float> rmax;
	rmax.resize(one.width * one.height);
	for (int j = 0; j < one.height; j++) {
		for (int i = 0; i < one.width; i++) {
			rmax[j * one.width + i] = fmaxf(one.vals[j * one.width + i], two.vals[j * one.width + i]);
		}
	}
	return Table(rmax, one.width, one.height);
}

class Lesson
{
private:
	static int dx(int field_w, int i)
	{
		int cx = (field_w - 1) / 2;
		return -(i - cx);
	}
	static int dy(int field_h, int j)
	{
		int cy = (field_h - 1) / 2;
		return -(j - cy);
	}

public:
	int field_w, field_h;
	float priority;
	Table position;

	Lesson(Table position, int field_w, int field_h, Point move, float priority) :
		field_w(field_w), field_h(field_h), priority(priority),
		position(position.translateRoll(dx(field_w, move.i), dy(field_h, move.j)))
	{
	}

	Lesson(Table position, int field_w, int field_h, int priority) :
		field_w(field_w), field_h(field_h), priority(priority),
		position(position)
	{
	}

	Lesson inverse() const
	{
		Lesson res(*this);

		res.position = position.inverse();

		return res;
	}

	Lesson rotateClockwise() const
	{
		Lesson res(*this);
		if (field_w != field_h)
		{
			throw runtime_error("Can't rotate a non-square field");
		}

		res.position = position.rotateClockwise();

		return res;
	}

	Lesson mirrorHorizontal() const
	{
		Lesson res(*this);

		res.position = position.mirrorHorizontal();

		return res;
	}

	Lesson mulPriority(float priority_mul) const
	{
		Lesson res(*this);

		res.priority *= priority_mul;

		return res;
	}

};

float score(network<sequential> net, Table field)
{
	int field_w = field.width;
	int field_h = field.height;

	vec_t pos_item;
	for (int j = 0; j < field_h; j++) {
		for (int i = 0; i < field_w; i++) {
			pos_item.push_back(field.vals[j * field_w + i]);
		}
	}

	vec_t prior = net.predict(pos_item);

	return prior[0];
}

/*void makeMove(network<sequential> net, Table field_data, Point& move, int& victor, bool rotate = true)
{
	Table ai_prior_0_0 = score(net, field_data);
	Table ai_prior(field_data.width, field_data.height);
	if (rotate) {

		Table field_90 = field_data.rotateClockwise();
		Table ai_prior_1_90 = score(net, field_90);

		Table field_180 = field_90.rotateClockwise();
		Table ai_prior_1_180 = ai_prior_1_90.rotateClockwise();
		Table ai_prior_2_180 = score(net, field_180);

		Table field_270 = field_180.rotateClockwise();
		Table ai_prior_1_270 = ai_prior_1_180.rotateClockwise();
		Table ai_prior_2_270 = ai_prior_2_180.rotateClockwise();
		Table ai_prior_3_270 = score(net, field_270);

		Table ai_prior_1_0 = ai_prior_1_270.rotateClockwise();
		Table ai_prior_2_0 = ai_prior_2_270.rotateClockwise();
		Table ai_prior_3_0 = ai_prior_3_270.rotateClockwise();
	
		Table ai_tmp1 = max(ai_prior_0_0, ai_prior_1_0);
		Table ai_tmp2 = max(ai_tmp1, ai_prior_2_0);
		ai_prior = max(ai_tmp2, ai_prior_3_0);
	}
	else
	{
		ai_prior = ai_prior_0_0;
	}

	// Searching for the maximum
	bool occupied;
	float maxpriority;
	do
	{
		occupied = false;
		move.i = -1; move.j = -1;
		maxpriority = -1.0;
		for (int j = 0; j < field_data.height; j++)
		{
			for (int i = 0; i < field_data.width; i++)
			{
				if (ai_prior.vals[j * field_data.width + i] > maxpriority)
				{
					maxpriority = ai_prior.vals[j * field_data.width + i];
					move.i = i;
					move.j = j;

					if (abs(field_data.vals[j * field_data.width + i]) > 0.5)
					{
						occupied = true;
						ai_prior.vals[j * field_data.width + i] = -1.0; // clearing this priority
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
}*/

Point makeMove(network<sequential> net, Table field_data)
{
	int cx = (field_data.width - 1) / 2;
	int cy = (field_data.height - 1) / 2;

	vector<float> res(field_data.width * field_data.height);
	std::fill(res.begin(), res.end(), -100.0f);	// As low as possible
	
	//printf("---> ");

	// Scoring for each cell's priority
	for (int dy = 0; dy < field_data.height; dy++)
	{
		for (int dx = 0; dx < field_data.width; dx++)
		{
			// Rolling the board
			Table rolled = field_data.translateRoll(dx, dy);

			if (abs(rolled.vals[cy * rolled.width + cx]) > 0.5)
			{
				//printf("%d,%d; ", dx, dy);
				continue;	// The cell is occupied
			}

			// Rotating & scoring
			float ai_prior_0_0 = score(net, rolled);
			float ai_prior_0_0m = score(net, rolled.mirrorHorizontal());

			Table field_90 = rolled.rotateClockwise();
			float ai_prior_1_90 = score(net, field_90);
			float ai_prior_1_90m = score(net, field_90.mirrorHorizontal());

			Table field_180 = field_90.rotateClockwise();
			float ai_prior_2_180 = score(net, field_180);
			float ai_prior_2_180m = score(net, field_180.mirrorHorizontal());

			Table field_270 = field_180.rotateClockwise();
			float ai_prior_3_270 = score(net, field_270);
			float ai_prior_3_270m = score(net, field_270.mirrorHorizontal());

			// TODO MIRRORING!!

			float ai_tmp = max(ai_prior_0_0, ai_prior_0_0m);
			ai_tmp = max(ai_tmp, ai_prior_1_90);
			ai_tmp = max(ai_tmp, ai_prior_1_90m);
			ai_tmp = max(ai_tmp, ai_prior_2_180);
			ai_tmp = max(ai_tmp, ai_prior_2_180m);
			ai_tmp = max(ai_tmp, ai_prior_3_270);
			ai_tmp = max(ai_tmp, ai_prior_3_270m);

			float ai_prior = ai_tmp;

			res[dy * field_data.width + dx] = ai_prior;
		}
	}

	// Searching for the hightest priority
	int maxdx = -1, maxdy = -1; float m = -1.0;
	for (int dx = 0; dx < field_data.width; dx++)
	{
		for (int dy = 0; dy < field_data.height; dy++)
		{
			if (res[dy * field_data.width + dx] > m)
			{
				m = res[dy * field_data.width + dx];
				maxdx = dx;
				maxdy = dy;
			}
		}
	}

	//printf(" ==> %d,%d; ", maxdx, maxdy);
	//printf("\n");

	Point move(
		(cx - maxdx + field_data.width) % field_data.width, 
		(cy - maxdy + field_data.height) % field_data.height, 
		field_data.width, field_data.height
	);

	//printf(" ==> %d,%d; ", move.i, move.j);
	//printf("\n");

	return move;
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

		float priority;
		lessonsFile.read((char*)&priority, 4);

		vector<float> position;
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				float f;
				lessonsFile.read((char*)&f, 4);
				position.push_back(f);
			}
		}

		usefulLessons.push_back(
			Lesson(
				Table(position, field_w, field_h), 
				field_w, field_h, 
				priority
			)
		);
	}
	lessonsFile.close();

/*	for (int i = 0; i < usefulLessons.size(); i++)
	{
		printField(usefulLessons[i].position.vals, usefulLessons[i].position.width, usefulLessons[i].position.height);
	}*/

	printf("Lessons in the book: %d\n", usefulLessons.size());

	size_t training_batch = usefulLessons.size();
	int batches_count = 1;

	// 1. Generating training & testing data

	vector<vec_t> train_input_data;
	vector<vec_t> train_output_data;

	for (int m = 0; m < batches_count * training_batch; ++m)
	{
		int k = m % usefulLessons.size();

		vec_t pos_item;
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				pos_item.push_back(usefulLessons[k].position.vals[j * field_w + i]);
			}
		}
		train_input_data.push_back(pos_item);

		vec_t pri_item { usefulLessons[k].priority };
		train_output_data.push_back(pri_item);
	}


	printf("Training...\n");

	size_t batch_size = std::min((int)train_input_data.size(), 20);//training_batch;
	double loss = 0; int ee = 0;
	
	double delta_loss_per_epoch;
	
	//gradient_descent opt; opt.alpha = 0.75;
	adam opt; opt.alpha /= 5;
	//int succeeded_tests;

	size_t epochs = 200;
	loss = net.get_loss<mse>(train_input_data, train_output_data);
	do
	{
		net.fit<mse>(opt, train_input_data, train_output_data, batch_size, epochs);

		double old_loss = loss;
		loss = net.get_loss<mse>(train_input_data, train_output_data);

		delta_loss_per_epoch = (old_loss - loss) / epochs;
		if (delta_loss_per_epoch < 0) opt.alpha /= 2;

		// Scoring

		int succeeded_tests = 0;
		for (int i = 0; i < usefulLessons.size(); i++)
		{
			Table field_data = usefulLessons[i].position;

			Point move = makeMove(net, field_data);

			if (move.i == (field_data.width - 1) / 2 &&
				move.j == (field_data.width - 1) / 2)
			{
				succeeded_tests++;
				//printField(usefulLessons[i].position.vals,usefulLessons[i].position.width, usefulLessons[i].position.height);
			}
		}

		ee += epochs;
		cout << "epoch " << ee << ": loss=" << loss << " dloss=" << delta_loss_per_epoch << "; alpha=" << opt.alpha << "; learned: " << succeeded_tests << " of " << usefulLessons.size() << "(" << (int)(succeeded_tests * 100 / usefulLessons.size()) << "%)" << endl;

	} while (delta_loss_per_epoch > mse_stop || delta_loss_per_epoch < 0);
}

int main(int argc, char** argv)
{
	std::cout << "NN backend: " << core::default_engine() << std::endl;

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

		int conv_kernel = 2;
		int conv1_out_w = field_w - conv_kernel + 1;
		int conv1_out_h = field_h - conv_kernel + 1;
		int conv1_out = conv1_out_w * conv1_out_h;
		int maps = conv_kernel * conv_kernel;	// from the ceiling

		int conv_kernel2 = 2;
		int conv2_out_w = conv1_out_w - conv_kernel2 + 1;
		int conv2_out_h = conv1_out_h - conv_kernel2 + 1;
		int conv2_out = conv2_out_w * conv2_out_h;
		int maps2 = maps* conv_kernel2 * conv_kernel2;	// from the ceiling




		net << layers::conv(field_w, field_h, conv_kernel, 1, maps) <<

			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<
			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<

			layers::conv(conv1_out_w, conv1_out_h, conv_kernel2, maps, maps2) <<

			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<
			layers::fc(conv2_out * maps2, conv2_out * maps) << tanh_layer(conv2_out * maps) <<

			layers::fc(conv2_out * maps, 1) << tanh_layer(1);
	}

	if (argc == 2 && strcmp(argv[1], "train-first") == 0)
	{
		train(field_w, field_h, net, 1e-5);
	}


	vector<Lesson> lessons[2];	// Each player's lessons

	Table field_data(field_w, field_h);
	for (int j = 0; j < field_h; j++) {
		for (int i = 0; i < field_w; i++) {
			field_data.vals.push_back(0.0);
		}
	}

	srand(time(nullptr));
	int userPlayerIndex = rand() % 2;

	int victor = -2;
	do {
		for (int pI = 0; pI < 2; pI++) {
			printField(field_data.vals, field_w, field_h);
			float vic = checkVictory(field_data.vals, field_w, field_h, vic_line_len);
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

			Table field_data_me_him = field_data;
			if (userPlayerIndex != 1) {
				// If the user plays "X", we inverse the field because "X" should mean "me"
				field_data_me_him = field_data_me_him.inverse();
			}

			Point move(0, 0, field_w, field_h);
			if (pI == userPlayerIndex)
			{
				// Player move
				do {
					printf("%c>", playerSymbol(pI));
					char x = getchar();
					char y = getchar();
					char c;  while ((c = getchar()) != '\n' && c != EOF) {}
					move.i = x - 'a';
					move.j = y - '1';
				} while (move.i >= field_w || move.j > field_h || move.i < 0 || move.j < 0);

			}
			else
			{
				// AI move
				move = makeMove(net, field_data_me_him);
			}

			if (victor != -1) {
				// If not draw
				lessons[pI].push_back(Lesson(field_data_me_him, field_w, field_h, move, 1.0));
				field_data.vals[move.j * field_w + move.i] = playerVal(pI);
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
			if (victor != 0 /* X */) cur = cur.inverse();

			usefulLessons.push_back(cur);

			Lesson curi = cur.inverse().mulPriority(0.95);		// Defense is less prioritized than attack
			usefulLessons.push_back(curi);
			
			priority /= 1.5;
		}

		int looser = (victor + 1) % 2;
		priority = -0.2;
		for (int k = lessons[looser].size() - 1; k >= 0; k--)
		{
			Lesson cur = lessons[looser][k].mulPriority(priority);

			// In the book 1 means "me" and -1 means "other player", not "X" and "O"
			if (looser != 0 /* X */) cur = cur.inverse();

			usefulLessons.push_back(cur);

			Lesson curi = cur.inverse().mulPriority(0.95);		// Defense is less prioritized than attack
			usefulLessons.push_back(curi);

			priority /= 1.5;
		}


		// Saving useful lessons to file
		ofstream lessonsFile;
		lessonsFile.open("lessons.dat", ios::app | ios::binary | ios::out);
		for (int k = 0; k < usefulLessons.size(); k++)
		{
			lessonsFile.write("L", 1);	// Magic for a lesson

			lessonsFile.write((const char*) &(usefulLessons[k].priority), 4);
			for (int j = 0; j < field_h; j++) {
				for (int i = 0; i < field_w; i++) {
					lessonsFile.write((const char*) &(usefulLessons[k].position.vals[j * field_w + i]), 4);
				}
			}
		}
		lessonsFile.close();

		printf("%d lessons appended to the book\n", usefulLessons.size());

		train(field_w, field_h, net, 1e-5);

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
