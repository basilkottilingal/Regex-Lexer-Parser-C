/*
.. Given an NFA (Q, Σ, δ, q0 , F ) there exists
.. atleast one partition P (Σ), which satisfy the condition ( cond-1 )
..   ∀ p ∈ P, and c, c′ ∈ p   ⇒   δ(q, c) = δ(q, c′)  ∀  q ∈ Q
.. Now, the equivalence class is the function
..   C(c) : Σ → {0, 1, ..., |P | − 1} which maps c to p
.. i.e.
..   C(c) = i ⇐⇒ c ∈ p
.. Our aim is to look for the coarsest partition, or the one
.. that satisfy above condition (cond-1) and minimises |P(Σ)|.
*/

#define ALPH 256

static uint8_t class [ALPH];

int class_init () {
  memset (class, 0, sizeof (class));
}

/*
void refine_class(int *is_selected) {
  int old_to_new[ALPHABET];
  memset(old_to_new, -1, sizeof(old_to_new));
  int new_class = num_classes;
  for (int c = 0; c < ALPHABET; c++) {
    int cls = equiv_class[c];
    int select = is_selected[c];
    if (select) {
      // If first time splitting this class
      if (old_to_new[cls] == -1) {
        old_to_new[cls] = new_class++;
      }
      equiv_class[c] = old_to_new[cls];
    }
  }
  num_classes = new_class;
}
*/

void refine_class(int *is_selected) {
    int old_to_new[ALPHABET];
    memset(old_to_new, -1, sizeof(old_to_new));

    int new_class = num_classes;

    for (int cls = 0; cls < num_classes; cls++) {
        int count_in = 0, count_total = 0;
        for (int c = 0; c < ALPHABET; c++) {
            if (equiv_class[c] == cls) {
                count_total++;
                if (is_selected[c]) count_in++;
            }
        }
        if (count_in == 0 || count_in == count_total)
            continue;  // nothing to split

        // Actually split: assign new class to selected chars
        for (int c = 0; c < ALPHABET; c++)
            if (equiv_class[c] == cls && is_selected[c])
                equiv_class[c] = new_class;
        new_class++;
    }

    num_classes = new_class;
}

