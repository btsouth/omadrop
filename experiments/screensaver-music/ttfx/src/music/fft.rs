//! In-place radix-2 FFT, unnormalized like FFTW's forward transform, so the
//! analyzer's magnitudes match Omadrop's FFTW-based AudioFeatureBus.

pub struct Fft {
    n: usize,
    cos: Vec<f32>,
    sin: Vec<f32>,
    rev: Vec<u32>,
}

impl Fft {
    pub fn new(n: usize) -> Self {
        assert!(n.is_power_of_two());
        let bits = n.trailing_zeros();
        let rev = (0..n as u32)
            .map(|i| i.reverse_bits() >> (32 - bits))
            .collect();
        let (mut cos, mut sin) = (Vec::with_capacity(n / 2), Vec::with_capacity(n / 2));
        for k in 0..n / 2 {
            let a = -2.0 * std::f64::consts::PI * k as f64 / n as f64;
            cos.push(a.cos() as f32);
            sin.push(a.sin() as f32);
        }
        Fft { n, cos, sin, rev }
    }

    /// Forward transform of `re` (imaginary part zero on input).
    pub fn forward(&self, re: &mut [f32], im: &mut [f32]) {
        let n = self.n;
        for i in 0..n {
            let j = self.rev[i] as usize;
            if j > i {
                re.swap(i, j);
                im.swap(i, j);
            }
        }
        let mut size = 2;
        while size <= n {
            let half = size / 2;
            let step = n / size;
            for start in (0..n).step_by(size) {
                for k in 0..half {
                    let (c, s) = (self.cos[k * step], self.sin[k * step]);
                    let (a, b) = (start + k, start + k + half);
                    let tr = re[b] * c - im[b] * s;
                    let ti = re[b] * s + im[b] * c;
                    re[b] = re[a] - tr;
                    im[b] = im[a] - ti;
                    re[a] += tr;
                    im[a] += ti;
                }
            }
            size *= 2;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sine_lands_in_its_bin() {
        let n = 2048;
        let fft = Fft::new(n);
        let mut re: Vec<f32> = (0..n)
            .map(|i| (2.0 * std::f32::consts::PI * 64.0 * i as f32 / n as f32).cos())
            .collect();
        let mut im = vec![0.0; n];
        fft.forward(&mut re, &mut im);
        let mag = |k: usize| (re[k] * re[k] + im[k] * im[k]).sqrt();
        assert!((mag(64) - n as f32 / 2.0).abs() < 1.0);
        assert!(mag(63) < 0.01 && mag(65) < 0.01);
    }
}
