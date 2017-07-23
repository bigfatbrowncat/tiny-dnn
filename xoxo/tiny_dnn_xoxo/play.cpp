#include <stdio.h>

#include "tiny_dnn/tiny_dnn.h"

#include <memory>
#include <vector>
#include <fstream>

#include "Table.h"

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

			if (field_data[(j * field_w + i) * 2 + 0] > 0.5) {
				printf(" X");
			}
			else if (field_data[(j * field_w + i) * 2 + 1] > 0.5)
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

void printPriors(vector<float> field_data, int field_w, int field_h)
{
	for (int j = 0; j < field_h; j++)
	{
		for (int i = 0; i < field_w; i++)
		{
			if (field_data[j * field_w + i] < -99) {
				printf("XXXX ");
			} else {
				printf("%.2f ", field_data[j * field_w + i]);
			}
		}
		printf("\n");
	}
	printf("\n");
}


bool playerCond(int pI, float* val) {
	return val[pI] > 0.5;
}

//float playerVal(int pI) {
//	if (pI == 0 /*X*/) return 1.0;
//	else if (pI == 1 /*O*/) return -1.0;
//	else throw 0;
//}

char playerSymbol(int pI) {
	if (pI == 0 /*X*/) return 'X';
	else if (pI == 1 /*O*/) return 'O';
	else throw 0;
}

int checkVictory(vector<float> field_data, int field_w, int field_h, int line_len) {
	for (int j = 0; j < field_h + line_len - 1; j++) {
		for (int i = 0; i < field_w + line_len - 1; i++) {
			int i1 = i % field_w;
			int j1 = j % field_h;

			// Iterate thru players - 0=X 1=O
			for (int pI = 0; pI < 2; pI++) {
				int cnt;
				// Checking player victory
				if (playerCond(pI, &field_data[(j1 * field_w + i1) * 2])) {

					// Right
					cnt = 1;
					for (int p = 1; p < line_len; p++) {
						int i2 = (i + p) % field_w;
						if (playerCond(pI, &field_data[(j1 * field_w + i2) * 2])) cnt++;
					}
					if (cnt == line_len)
						return pI;

					// Down
					cnt = 1;
					for (int q = 1; q < line_len; q++) {
						int j2 = (j + q) % field_h;
						if (playerCond(pI, &field_data[(j2 * field_w + i1) * 2])) cnt++;
					}
					if (cnt == line_len) 
						return pI;

					// Right-down
					cnt = 1;
					for (int pq = 1; pq < line_len; pq++) {
						int i2 = (i + pq) % field_w;
						int j2 = (j + pq) % field_h;
						if (playerCond(pI, &field_data[(j2 * field_w + i2) * 2])) cnt++;
					}
					if (cnt == line_len)
						return pI;

					// Left-down
					cnt = 1;
					for (int pq = 1; pq < line_len; pq++) {
						int i2 = (i - pq + field_w) % field_w;
						int j2 = (j + pq) % field_h;
						if (playerCond(pI, &field_data[(j2 * field_w + i2) * 2])) cnt++;
					}
					if (cnt == line_len)
						return pI;
				}
			}

		}
	}
	return -1; // No victory found
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



vector<float> max(vector<float> one, vector<float> two)
{
	assert(one.size() == two.size());

	vector<float> rmax;
	rmax.resize(one.size());
	for (int j = 0; j < one.size(); j++) {
		rmax[j] = fmaxf(one[j], two[j]);
	}
	return rmax;
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
	Table answer;
	Table position;

	/*Lesson(Table position, int field_w, int field_h, Point move) :
		field_w(field_w), field_h(field_h), answer(Table::empty(field_w, field_h, 1)),
		position(position.translateRoll(dx(field_w, move.i), dy(field_h, move.j)))
	{
		
	}*/

	Lesson(Table position, int field_w, int field_h, Point move) :
		field_w(field_w), field_h(field_h), answer(Table::empty(field_w, field_h, 1)),
		position(position)
	{
		answer.vals[move.j * field_w + move.i] = 1.0;
	}

	Lesson(Table position, int field_w, int field_h, Table answer) :
		field_w(field_w), field_h(field_h), answer(answer),
		position(position)
	{
	}

	Lesson inverseChannels() const
	{
		Lesson res(*this);

		res.position = position.inverseChannels();

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
		res.answer = answer.rotateClockwise();

		return res;
	}

	Lesson mirrorHorizontal() const
	{
		Lesson res(*this);

		res.position = position.mirrorHorizontal();
		res.answer = answer.mirrorHorizontal();

		return res;
	}

	Lesson setAnswerToZero() const
	{
		Lesson res(*this);

		res.answer = Table::empty(answer.width, answer.height, answer.channels);

		return res;
	}

	Lesson setAnswerToValueAt(int i, int j, float value) const
	{
		Lesson res(*this);

		res.answer = Table::empty(answer.width, answer.height, answer.channels);
		res.answer.vals[(j * answer.width + i) * answer.channels + 0] = value;

		return res;
	}

};

Point makeMove(network<sequential> net, Table field_data, bool rotate_and_mirror = true)
{
	int extDepth = 3;

	vector<float> resVec(field_data.width * field_data.height * 1/* channels */);
	std::fill(resVec.begin(), resVec.end(), -100.0f);	// As low as possible
	
	Table res(resVec, field_data.width, field_data.height, 1);

	Table extended = field_data.extendToroidal(extDepth);

	Table ai_prior(field_data.width, field_data.height, 1);

	if (rotate_and_mirror)
	{
		// Rotating & scoring
		vector<Table> scoredRotated;
		for (int angle = 0; angle < 4; angle++) {
			for (auto& sR : scoredRotated) {
				sR = sR.rotateClockwise();
			}

			Table ai_prior_cur = Table::score(net, extended);
			Table ai_prior_cur_m = Table::score(net, extended.mirrorHorizontal()).mirrorHorizontal();
			scoredRotated.push_back(ai_prior_cur);
			scoredRotated.push_back(ai_prior_cur_m);
		}
		for (auto& sR : scoredRotated) {
			sR = sR.rotateClockwise();
		}

		Table ai_tmp = scoredRotated[0];
		for (auto& sR : scoredRotated) {
			ai_tmp = Table::max(ai_tmp, sR);
		}

		ai_prior = ai_tmp;

	}
	else
	{
		ai_prior = Table::score(net, extended);
	}

	res = ai_prior;

	//res[dy * field_data.width + dx] = ai_prior.vals[dy * field_data.width + dx];

/*	vector<float> show = res.vals;
	for (int dy = 0; dy < field_data.height; dy++)
	{
		for (int dx = 0; dx < field_data.width; dx++)
		{
			int xx = (-dx + cx + field_data.width) % field_data.width;
			int yy = (-dy + cy + field_data.height) % field_data.height;

			show[yy * field_data.width + xx] = res.vals[dy * field_data.width + dx];
		}
	}
*/	//printPriors(show, field_data.width, field_data.height);

	// Searching for the hightest priority
	int maxi = -1, maxj = -1; float m = -1.0;
	for (int i = 0; i < field_data.width; i++)
	{
		for (int j = 0; j < field_data.height; j++)
		{
			int x = i + extDepth;
			int y = j + extDepth;

			if (res.vals[y * field_data.width + x] > m && 
				abs(field_data.vals[(j * field_data.width + i) * 2 + 0]) < 0.5 &&
				abs(field_data.vals[(j * field_data.width + i) * 2 + 1]) < 0.5)
			{
				m = res.vals[j * field_data.width + i];
				maxi = i;
				maxj = j;
			}
		}
	}

	//printf(" ==> %d,%d; ", maxdx, maxdy);
	//printf("\n");

	Point move(maxi, maxj, field_data.width, field_data.height);

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

		vector<float> position;
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				for (int c = 0; c < 2; c++) {
					float f;
					lessonsFile.read((char*)&f, 4);
					position.push_back(f);
				}
			}
		}

		vector<float> answer;
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				for (int c = 0; c < 1; c++) {
					float f;
					lessonsFile.read((char*)&f, 4);
					answer.push_back(f);
				}
			}
		}

		usefulLessons.push_back(
			Lesson(
				Table(position, field_w, field_h, 2), 
				field_w, field_h, 
				Table(answer, field_w, field_h, 1)
			)
		);
	}
	lessonsFile.close();

	// Adding "zero" lesson
	usefulLessons.push_back(
		Lesson(
			Table::empty(field_w, field_h, 2), 
			field_w, field_h, 
			Table::empty(field_w, field_h, 1)
		)
	);


/*	for (int i = 0; i < usefulLessons.size(); i++)
	{
		printField(usefulLessons[i].position.vals, usefulLessons[i].position.width, usefulLessons[i].position.height);
	}*/

	printf("Lessons in the book: %d\n", usefulLessons.size());

	// 1. Generating training & testing data

	vector<vec_t> train_input_data;
	vector<vec_t> train_output_data;

	int repeats = 1;

	int extDepth = 3;

	for (int repeat = 0; repeat < repeats; repeat++)
	{
		// Adding the lessons
		for (int k = 0; k < usefulLessons.size(); ++k)
		{
			vec_t pos_item = usefulLessons[k].position.extendToroidal(extDepth).toVec();
			train_input_data.push_back(pos_item);

			vec_t pri_item = usefulLessons[k].answer.toVec();
			train_output_data.push_back(pri_item);
		}

		// Adding some fake lessons
/*		for (int k = 0; k < usefulLessons.size(); ++k)
		{
			//if (usefulLessons[k].priority < 0.0) continue; // No falses for negative lessons

			Lesson falseLesson = usefulLessons[k % usefulLessons.size()];
			falseLesson.setClasses(vector<float>(strategic_depth * 2, 0.0));
			falseLesson.position = falseLesson.position.translateRoll(
				rand() % (falseLesson.field_w - 2) + 1,
				rand() % (falseLesson.field_h - 2) + 1
			);

			train_input_data.push_back(falseLesson.position.toVec());

			vec_t pri_item;
			for (int d = 0; d < strategic_depth * 2; d++) pri_item.push_back(falseLesson.classes[d]);
			train_output_data.push_back(pri_item);
		}*/

	}

	printf("Training...\n");

	size_t batch_size = std::min((int)train_input_data.size(), 5);//training_batch;
	double loss = 0; int ee = 0;
	
	double delta_loss_per_epoch;
	
	//gradient_descent opt; opt.alpha = 0.75;
	adam opt; opt.alpha /= 30;
	//int succeeded_tests;

	int epochs = 100;
	loss = net.get_loss<mse>(train_input_data, train_output_data);
	do
	{
		net.fit<mse>(opt, train_input_data, train_output_data, batch_size, epochs);

		double old_loss = loss;
		loss = net.get_loss<mse>(train_input_data, train_output_data);

		delta_loss_per_epoch = (old_loss - loss) / epochs;
		//if (delta_loss_per_epoch < 0) opt.alpha /= 2;

		ee += epochs;
		cout << "epoch " << ee << ": loss=" << loss << " dloss=" << delta_loss_per_epoch << "; alpha=" << opt.alpha << endl;

		int succeeded_tests = 0;
		for (int i = 0; i < usefulLessons.size(); i++)
		{
			Table field_data = usefulLessons[i].position;

			Point move = makeMove(net, field_data, false);

			bool hit_the_point = usefulLessons[i].answer.vals[(move.j * field_data.width + move.i) * 1 + 0] > 0.5;
			/*	move.i == (field_data.width - 1) / 2 &&
				move.j == (field_data.height - 1) / 2;*/

			float smallEpsilon = 0.0001;

			float lessonAmplitude = 1.0f; // Only 1

			if ((abs(lessonAmplitude) < smallEpsilon) ||
				(lessonAmplitude > 0 && hit_the_point) ||
				(lessonAmplitude < 0 && !hit_the_point))
			{
				succeeded_tests++;
				/*printf("SUCCESS\n");
				printField(usefulLessons[i].position.vals,usefulLessons[i].position.width, usefulLessons[i].position.height);*/
			}
			else
			{
				/*printf("FAIL\n");
				printField(usefulLessons[i].position.vals, usefulLessons[i].position.width, usefulLessons[i].position.height);*/
			}
		}

		cout << "Probing: " << succeeded_tests << " of " << usefulLessons.size() << "(" << (int)(succeeded_tests * 100 / usefulLessons.size()) << "%)" << endl;

	} while (delta_loss_per_epoch > mse_stop/* || delta_loss_per_epoch < 0*/);
}

int main(int argc, char** argv)
{
	std::cout << "NN backend: " << core::default_engine() << std::endl;

	const int field_w = 7, field_h = 7, vic_line_len = 4;
	const int strategic_depth = vic_line_len + 1;

	network<sequential> net;
	try
	{
		net.load("xoxonet.weights");
		printf("Net loaded\n");
	}
	catch (nn_error&)
	{
		printf("Can't load the net. Creating a new one\n");

		int extDepth = 3;

		int extField_w = field_w + extDepth * 2;
		int extField_h = field_h + extDepth * 2;

		int size = field_w * field_h;


		int conv_kernel = 4;
		int conv1_out_w = extField_w - conv_kernel + 1;
		int conv1_out_h = extField_h - conv_kernel + 1;
		int conv1_out = conv1_out_w * conv1_out_h;
		int maps = 2 * conv_kernel * conv_kernel;	// from the ceiling

		/*int conv_kernel2 = 2;
		int conv2_out_w = conv1_out_w - conv_kernel2 + 1;
		int conv2_out_h = conv1_out_h - conv_kernel2 + 1;
		int conv2_out = conv2_out_w * conv2_out_h;
		int maps2 = maps * conv_kernel2 * conv_kernel2;	// from the ceiling*/




		net <<

			layers::conv(extField_w, extField_h, conv_kernel, 2, maps) <<

			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<
			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<
			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<
			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<
			/*layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<
			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<*/

			/*layers::conv(conv1_out_w, conv1_out_h, conv_kernel2, maps, maps2) <<

			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<
			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<
			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<
			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<
			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<
			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<*/

			layers::fc(conv1_out * maps, size) << tanh_layer(size);
	}

	if (argc == 2 && strcmp(argv[1], "train-first") == 0)
	{
		train(field_w, field_h, net, 1e-3);
	}


	vector<Lesson> lessons[2];	// Each player's lessons

	Table field_data(Table::empty(field_w, field_h, 2));

	srand(time(nullptr));
	int userPlayerIndex = rand() % 2;

	int victor = -2; int moveId = 0;
	do {
		for (int pI = 0; pI < 2; pI++) {
			printField(field_data.vals, field_w, field_h);
			int vic = checkVictory(field_data.vals, field_w, field_h, vic_line_len);
			if (vic == 0)
			{
				printf("Player X wins\n");
				victor = 0;
				break;
			}
			else if (vic == 1)
			{
				printf("Player O wins\n");
				victor = 1;
				break;
			}

			Table field_data_me_him = field_data;
			if (pI != 0) {
				// If the current player isn't "X", we inverse the field because "X" should mean "me"
				field_data_me_him = field_data_me_him.inverseChannels();
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
				if (moveId > 0)
				{
					lessons[pI].push_back(
						Lesson(
							field_data_me_him, 
							field_w, field_h, 
							move
						)
					);	// Classes set to zeros
				}
				field_data.vals[(move.j * field_w + move.i) * 2 + pI] = 1.0;
			}
			moveId++;
		}

	} while (victor < -1);

	if (victor == userPlayerIndex) {
		printf("Other player wins. I shall learn\n");

		vector<Lesson> usefulLessons;

		for (int k = lessons[victor].size() - 1; k >= max((int)lessons[victor].size() - 1 - strategic_depth, 0); k--)
		{
			int i = lessons[victor].size() - k - 1;

			Lesson cur = lessons[victor][k];// .setAnswerToValueAt(field_w / 2, field_h / 2, 1.0);
			usefulLessons.push_back(cur);
			
			Lesson curi = lessons[victor][k].inverseChannels();// .setClasses(classesDefense);
			usefulLessons.push_back(curi);
		}

		
		/*int looser = (victor + 1) % 2;
		for (int k = lessons[looser].size() - 1; k >= max((int)lessons[looser].size() - 1 - strategic_depth, 0); k--)
		{
			int i = lessons[looser].size() - k - 1;

			vector<float> classes(strategic_depth * 2, 0.0);
			classes[i * 2 + 1] = -1.0;
			Lesson cur = lessons[looser][k].setClasses(classes);
			usefulLessons.push_back(cur);

		}*/


		// Saving useful lessons to file
		ofstream lessonsFile;
		lessonsFile.open("lessons.dat", ios::app | ios::binary | ios::out);
		for (int k = 0; k < usefulLessons.size(); k++)
		{
			lessonsFile.write("L", 1);	// Magic for a lesson

			for (int j = 0; j < field_h; j++) {
				for (int i = 0; i < field_w; i++) {
					for (int c = 0; c < 2; c++) {
						lessonsFile.write((const char*) &(usefulLessons[k].position.vals[(j * field_w + i) * 2 + c]), 4);
					}
				}
			}

			for (int j = 0; j < field_h; j++) {
				for (int i = 0; i < field_w; i++) {
					for (int c = 0; c < 1; c++) {
						lessonsFile.write((const char*) &(usefulLessons[k].answer.vals[(j * field_w + i) * 1 + c]), 4);
					}
				}
			}

		}
		lessonsFile.close();

		printf("%d lessons appended to the book\n", usefulLessons.size());

		train(field_w, field_h, net, 1e-3);

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
